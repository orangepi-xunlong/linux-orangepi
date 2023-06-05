/*
 * sound\soc\sunxi\snd_sunxi_log.h
 * (C) Copyright 2021-2025
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_LOG_H
#define __SND_SUNXI_LOG_H
#include <linux/kernel.h>

#define SND_LOG_ERR(head, fmt, arg...) \
	pr_err("[sound %4d][" head " %s] " fmt, __LINE__, __func__, ##arg)

#define SND_LOG_WARN(head, fmt, arg...) \
	pr_warn("[sound %4d][" head " %s] " fmt, __LINE__, __func__, ##arg)

#define SND_LOG_INFO(head, fmt, arg...) \
	pr_info("[sound %4d][" head " %s] " fmt, __LINE__, __func__, ##arg)

#define SND_LOG_DEBUG(head, fmt, arg...) \
	pr_debug("[sound %4d][" head " %s] " fmt, __LINE__, __func__, ##arg)

#endif /* __SND_SUNXI_LOG_H */


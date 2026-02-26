/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2021, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_LOG_H
#define __SND_SUNXI_LOG_H

#ifndef SUNXI_MODNAME
#define SUNXI_MODNAME		"sound-"
#endif

#include <sunxi-log.h>

#if IS_ENABLED(CONFIG_AW_LOG_VERBOSE)
#define SND_LOG_ERR(fmt, arg...)		sunxi_err(NULL,   fmt, ##arg)
#define SND_LOG_WARN(fmt, arg...) 		sunxi_warn(NULL,  fmt, ##arg)
#define SND_LOG_INFO(fmt, arg...)		sunxi_info(NULL,  fmt, ##arg)
#define SND_LOG_DEBUG(fmt, arg...)		sunxi_debug(NULL, fmt, ##arg)
#define SND_LOG_ERR_STD(err, fmt, arg...)	sunxi_err_std(NULL, err, fmt, ##arg)

#define SND_LOGDEV_ERR(dev, fmt, arg...)	sunxi_err(dev,   fmt, ##arg)
#define SND_LOGDEV_WARN(dev, fmt, arg...)	sunxi_warn(dev,  fmt, ##arg)
#define SND_LOGDEV_INFO(dev, fmt, arg...)	sunxi_info(dev,  fmt, ##arg)
#define SND_LOGDEV_DEBUG(dev, fmt, arg...)	sunxi_debug(dev, fmt, ##arg)
#define SND_LOG_ERR_STD(dev, err, fmt, arg...)	sunxi_err_std(dev, err, fmt, ##arg)
#else
#define SND_LOG_ERR(fmt, arg...)		\
	sunxi_err(NULL,   "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOG_WARN(fmt, arg...) 		\
	sunxi_warn(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOG_INFO(fmt, arg...)		\
	sunxi_info(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOG_DEBUG(fmt, arg...)		\
	sunxi_debug(NULL, "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOG_ERR_STD(err, fmt, arg...)		\
	sunxi_err_std(NULL, err, "%d %s(): "fmt, __LINE__, __func__, ## arg)

#define SND_LOGDEV_ERR(dev, fmt, arg...)	\
	sunxi_err(dev,   "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOGDEV_WARN(dev, fmt, arg...)	\
	sunxi_warn(dev,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOGDEV_INFO(dev, fmt, arg...)	\
	sunxi_info(dev,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOGDEV_DEBUG(dev, fmt, arg...)	\
	sunxi_debug(dev, "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define SND_LOGDEV_ERR_STD(dev, err, fmt, arg...)		\
	sunxi_err_std(dev, err, "%d %s(): "fmt, __LINE__, __func__, ## arg)
#endif

#endif /* __SND_SUNXI_LOG_H */

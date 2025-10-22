/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Sunxi SD/MMC host driver
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 * Author: lixiang <lixiang@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SUNXI_MMC_DEBUG_H__
#define __SUNXI_MMC_DEBUG_H__
#include <linux/clk.h>
#include <linux/reset/sunxi.h>

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <sunxi-log.h>

void sunxi_mmc_dumphex32(struct sunxi_mmc_host *host, char *name, void __iomem *base,
			 int len);
void sunxi_mmc_dump_des(struct sunxi_mmc_host *host, char *base, int len);

int sunxi_mmc_uperf_stat(struct sunxi_mmc_host *host,
						struct mmc_data *data,
						struct mmc_request *mrq_busy,
						bool bhalf);

int mmc_create_sys_fs(struct sunxi_mmc_host *host,
		      struct platform_device *pdev);
void mmc_remove_sys_fs(struct sunxi_mmc_host *host,
		       struct platform_device *pdev);
void sunxi_dump_reg(struct mmc_host *mmc);
int sunxi_remap_readl(resource_size_t  phy_addr, u32 *pval);
int sunxi_remap_writel(resource_size_t  phy_addr, u32 val);


//#define CONFIG_AW_MMC_DEBUG

#ifdef CONFIG_AW_MMC_DEBUG

#define SM_DEBUG_WARN	BIT(0)
#define SM_DEBUG_INFO	BIT(1)
#define SM_DEBUG_DBG	BIT(2)


#define SM_MSG(___mh, ___mpd, ...)	do {___mh = ___mh;  dev_err(&___mpd->dev, __VA_ARGS__); } while (0)
#define SM_MORE(___md, ___mpd, ...)	do {dev_err(&___mpd->dev, __VA_ARGS__);\
					    dev_err(&___mpd->dev, ":%s(L%d)", __FUNCTION__, __LINE__); \
					    } while (0)

#define SM_ERR(___d, ...)   do { struct platform_device *___pd = to_platform_device(___d);\
				struct mmc_host *___m = platform_get_drvdata(___pd);\
				struct sunxi_mmc_host *___h = mmc_priv(___m);\
				SM_MSG(___h, ___pd, "[E]"__VA_ARGS__); \
				} while (0)

#define SM_WARN(___d, ...)   do { struct platform_device *___pd = to_platform_device(___d);\
				struct mmc_host *___m = platform_get_drvdata(___pd);\
				struct sunxi_mmc_host *___h = mmc_priv(___m);\
				if ((___h)->debuglevel & SM_DEBUG_WARN) 	\
				    SM_MSG(___h, ___pd, "[W]"__VA_ARGS__); } while (0)

#define SM_INFO(___d, ...)   do { struct platform_device *___pd = to_platform_device(___d);\
				struct mmc_host *___m = platform_get_drvdata(___pd);\
				struct sunxi_mmc_host *___h = mmc_priv(___m);\
				if ((___h)->debuglevel & SM_DEBUG_INFO) 	\
				    SM_MSG(___h, ___pd, "[I]"__VA_ARGS__); } while (0)
#define SM_DBG(___d, ...)    do { struct platform_device *___pd = to_platform_device(___d);\
				struct mmc_host *___m = platform_get_drvdata(___pd);\
				struct sunxi_mmc_host *___h = mmc_priv(___m);\
				if ((___h)->debuglevel & SM_DEBUG_DBG) 	\
				    SM_MORE(___h, ___pd, "[D]"__VA_ARGS__); } while (0)
#else
#define SM_ERR   sunxi_err
#define SM_WARN  sunxi_warn
#define SM_INFO  sunxi_info
#define SM_DBG	 sunxi_debug
#endif


#endif

/* SPDX-License-Identifier: GPL-2.0
 * aw882xx_log.h
 *
 * Copyright (c) 2020 AWINIC Technology CO., LTD
 *
 * Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __AW882XX_LOG_H__
#define __AW882XX_LOG_H__

/********************************************
 * print information control
 *******************************************/
#define AW_LOG_ERR
//#define AW_LOG_INFO
//#define AW_LOG_DEBUG

#ifdef AW_LOG_ERR
#define aw_dev_err(dev, format, ...) \
		pr_err("[Awinic][%s]%s: " format "\n", dev_name(dev), __func__, ##__VA_ARGS__)
#else
#define aw_dev_err(fmt, arg...) do {} while (0)
#endif

#ifdef AW_LOG_INFO
#define aw_dev_info(dev, format, ...) \
		pr_info("[Awinic][%s]%s: " format "\n", dev_name(dev), __func__, ##__VA_ARGS__)
#else
#define aw_dev_info(dev, format, ...) \
		pr_debug("[Awinic][%s]%s: " format "\n", dev_name(dev), __func__, ##__VA_ARGS__)
#endif

#ifdef AW_LOG_DEBUG
#define aw_dev_dbg(dev, format, ...) \
		pr_debug("[Awinic][%s]%s: " format "\n", dev_name(dev), __func__, ##__VA_ARGS__)
#else
#define aw_dev_dbg(dev, format, ...) \
		pr_debug("[Awinic][%s]%s: " format "\n", dev_name(dev), __func__, ##__VA_ARGS__)
#endif




#ifdef AW_LOG_ERR
#define aw_pr_err(format, ...) \
		pr_err("[Awinic]%s: " format "\n", __func__, ##__VA_ARGS__)
#else
#define aw_pr_err(fmt, arg...) do {} while (0)
#endif

#ifdef AW_LOG_INFO
#define aw_pr_info(format, ...) \
		pr_info("[Awinic]%s: " format "\n", __func__, ##__VA_ARGS__)
#else
#define aw_pr_info(format, ...) \
		pr_debug("[Awinic]%s: " format "\n", __func__, ##__VA_ARGS__)
#endif

#ifdef AW_LOG_DEBUG
#define aw_pr_dbg(format, ...) \
		pr_debug("[Awinic]%s: " format "\n", __func__, ##__VA_ARGS__)
#else
#define aw_pr_dbg(fmt, arg...) do {} while (0)
#endif




//#define DEBUG_LOG_LEVEL
#ifdef DEBUG_LOG_LEVEL
#define DBG(fmt, arg...) \
	pr_debug("AWINIC_BIN %s,line= %d,"fmt, __func__, __LINE__, ##arg)
#define DBG_ERR(fmt, arg...) \
	pr_err("AWINIC_BIN_ERR %s,line= %d,"fmt, __func__, __LINE__, ##arg)
#else
#define DBG(fmt, arg...) do {} while (0)
#define DBG_ERR(fmt, arg...) do {} while (0)
#endif



#endif


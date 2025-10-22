/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef _SUNXI_POWER_DEBUG_H_
#define _SUNXI_POWER_DEBUG_H_

#include <sunxi-log.h>

#define PMIC_ERR(format, args...)	sunxi_err(NULL, format, ##args)
#define PMIC_WARN(format, args...)	sunxi_warn(NULL, format, ##args)
#define PMIC_INFO(format, args...)	sunxi_info(NULL, format, ##args)
#define PMIC_DEBUG(format, args...)	sunxi_debug(NULL, format, ##args)

#define PMIC_DEV_ERR(dev, format, args...)	sunxi_err(dev, format, ##args)
#define PMIC_DEV_WARN(dev, format, args...)	sunxi_warn(dev, format, ##args)
#define PMIC_DEV_INFO(dev, format, args...)	sunxi_info(dev, format, ##args)
#define PMIC_DEV_DEBUG(dev, format, args...)	sunxi_debug(dev, format, ##args)


#endif /*  _SUNXI_POWER_DEBUG_H_ */

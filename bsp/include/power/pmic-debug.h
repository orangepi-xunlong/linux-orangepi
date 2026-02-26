/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on the tcs4838 driver and the previous tcs4838 driver
 */

#ifndef __PMIC_DEBUG_H
#define __PMIC_DEBUG_H

#include <sunxi-log.h>

#define PMIC_ERR(format, args...)	sunxi_err(NULL, format, ##args)
#define PMIC_WARN(format, args...)	sunxi_warn(NULL, format, ##args)
#define PMIC_INFO(format, args...)	sunxi_info(NULL, format, ##args)
#define PMIC_DEBUG(format, args...)	sunxi_debug(NULL, format, ##args)

#define PMIC_DEV_ERR(dev, format, args...)	sunxi_err(dev, format, ##args)
#define PMIC_DEV_WARN(dev, format, args...)	sunxi_warn(dev, format, ##args)
#define PMIC_DEV_INFO(dev, format, args...)	sunxi_info(dev, format, ##args)
#define PMIC_DEV_DEBUG(dev, format, args...)	sunxi_debug(dev, format, ##args)


#endif /*  __PMIC_DEBUG_H */

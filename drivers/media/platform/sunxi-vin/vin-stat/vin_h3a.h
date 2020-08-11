/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-stat/vin_h3a.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 ******************************************************************************
 *
 * vin_3a.h
 *
 * Hawkview ISP - vin_3a.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date      Description
 *
 *   3.0         Yang Feng    2016/03/11    VIDEO INPUT
 *
 *****************************************************************************
 */

#ifndef _VIN_3A_H_
#define _VIN_3A_H_

#include "../vin-isp/sunxi_isp.h"

/* Init and cleanup functions */
int vin_isp_h3a_init(struct isp_dev *isp);
void vin_isp_h3a_cleanup(struct isp_dev *isp);

#endif /*_VIN_3A_H_*/


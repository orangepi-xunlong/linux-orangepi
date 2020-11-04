/*
 * linux-4.9/drivers/media/platform/sunxi-vfe/config.h
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
 * header file
 * set/get config from sysconfig and ini/bin file
 * Author: raymonxiu
 *
 */

#ifndef __CONFIG__H__
#define __CONFIG__H__

#include "vfe.h"
#include "utility/cfg_op.h"

int fetch_config(struct vfe_dev *dev);
int read_ini_info(struct vfe_dev *dev, int isp_id, char *main_path);

#endif /* __CONFIG__H__ */

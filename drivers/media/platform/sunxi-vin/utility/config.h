
/*
 * config.h for device tree and sensor list parser.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CONFIG__H__
#define __CONFIG__H__

#include "../vin-video/vin_core.h"
#include "../vin.h"
#include "cfg_op.h"

int parse_modules_from_device_tree(struct vin_md *vind);

#endif /*__CONFIG__H__*/

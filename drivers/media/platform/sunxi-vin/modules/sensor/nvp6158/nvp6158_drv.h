/*
 * A header of V4L2 driver for nvp6158c cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn <zequnzhgeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

u32 gpio_i2c_write(u8 da, u8 reg, u8 val);
u32 gpio_i2c_read(u8 da, u8 reg);
int check_nvp6158_id(unsigned int dec);
int check_nvp6158_rev(unsigned int dec);
int nvp6158_init_hardware(int video_mode);

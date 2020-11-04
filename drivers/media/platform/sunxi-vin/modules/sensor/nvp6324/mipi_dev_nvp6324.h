/*
 * A V4L2 driver for nvp6324 cameras and AHD Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Li Huiyu <lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

u32 nvp6324_i2c_write(u8 da, u8 reg, u8 val);
u32 nvp6324_i2c_read(u8 da, u8 reg);
int check_id(unsigned int dec);
int check_rev(unsigned int dec);
void read_bank_value(void);
int nvp6324_init(int mode);

/*
 * arch/arm/mach-sun7i/include/mach/dram.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Kevin <kevin@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __DRAM_H__
#define __DRAM_H__

#include "mctl_par.h"
#include "mctl_reg.h"
#include "mctl_hal.h"
#include "../../pm_i.h"

__s32 standby_set_dram_crc_paras(__u32 enable, __u32 src, __u32 len);
__u32 standby_dram_crc(void);
unsigned int dram_power_save_process(void);
unsigned int dram_power_up_process(__dram_para_t *para);


#endif  /* __DRAM_H__ */


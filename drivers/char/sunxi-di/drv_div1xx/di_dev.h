/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _DI_DEV_H_
#define _DI_DEV_H_
u32 di_dev_get_ip_version(void);
void di_dev_set_reg_base(void __iomem *reg_base);
void di_dev_exit(void);
u32 di_dev_query_state_with_clear(void);
s32 di_dev_apply(void *client);
#endif

/*
 * linux/include/linux/sunxi_module.h
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: superm<superm@allwinnertech.com>
 *
 * allwinner sunxi modules manager interfaces.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef  __SUNXI_MACH_SUNXI_MODULE_H
#define  __SUNXI_MACH_SUNXI_MODULE_H

/*
 * @module : the pointer of module name string;
 * @version: the pointer of version string;
 */
typedef struct sunxi_module_info
{
	char module[16];
	char version[16];
}sunxi_module_info_t;

int sunxi_module_info_register(struct sunxi_module_info *module_info);
int sunxi_module_info_unregister(struct sunxi_module_info *module_info);

#endif /* __SUNXI_MACH_SUNXI_MODULE_H */


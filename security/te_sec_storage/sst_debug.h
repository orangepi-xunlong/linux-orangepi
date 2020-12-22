/* secure storage driver for sunxi platform 
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
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

#ifndef _SST_DEBUG_H_
#define _SST_DEBUG_H_

#include <linux/types.h>
#include <linux/kernel.h>

#include "sst_communicate.h"
#include "sst.h"

/*internal debug*/
extern int __data_dump( char * name ,void *data, unsigned int len);
extern int __desc_dump(store_desc_t  *desc);
extern int __object_dump(store_object_t  *object);
extern int __command_dump(cmd_t *cmd);
extern int __sst_map_dump(struct secure_storage_t  *sst);
extern int __oem_map_dump(struct secure_storage_t *sst);
extern int __cmd_work_dump(cmd_work_t *cmd);

extern void __param_dump(struct te_oper_param *param);
extern void __request_dump(struct te_request *r );

extern void *sst_memcpy(void *dest, const void *src, size_t count) ;
extern void *sst_memset(void *dest, int val, size_t count) ;

extern int sst_test_daemon_kthread(void *data);
#endif

/* secure storage driver for sunxi platform
 *
 * Copyright (C) 2014 Allwinner Ltd.
 *
 * Author:
 *	Ryan Chen <yanjianbo@allwinnertech.com>
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

#ifndef _SST_STORAGE_H_
#define _SST_STORAGE_H_

struct sst_storage_data{
	int id;
#ifdef CONFIG_COMPAT
	compat_uptr_t buf;
#else
    unsigned char *buf;
#endif
	unsigned int len;
	unsigned int offset;
};

struct fcrypt {
	struct sst_storage_data key_store;
	struct work_struct work;
	struct completion work_end;
	struct mutex mutex;
	unsigned char *temp_data;
	unsigned int cmd;
	unsigned int ret;
};

typedef struct fcrypt fcry_st;
typedef struct fcrypt *fcry_pt;

#define SECBLK_READ							_IO('V', 20)
#define SECBLK_WRITE						_IO('V', 21)
#define SECBLK_IOCTL						_IO('V', 22)

struct secblc_op_t{
	int item;
	unsigned char *buf;
	unsigned int len;
};

#endif

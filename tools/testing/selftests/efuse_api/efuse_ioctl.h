/*
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 * Author: superm <superm@allwinnertech.com>
 * Author: Matteo <duanmintao@allwinnertech.com>
 *
 * Allwinner sunxi soc chip version and chip id manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __EFUSE_IOCTRL_H__
#define __EFUSE_IOCTRL_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#define EFUSE_READ _IO('V', 1)
#define EFUSE_WRITE _IO('V', 2)
#define EFUSE_IOCTL_PATH ("/dev/sid_efuse")

typedef struct {
	char name[64];
	uint32_t len;
	uint32_t offset;
	uint64_t key_data;
} sunxi_efuse_key_info_t;

int efuse_data_read(char *key_name, int offset, int len, char *key_buf);
int efuse_data_write(char *key_name, int offset, char *key_buf);
int sunxi_efuse_read(char *key_name, int offset, int len, char *key_buf);
int sunxi_efuse_write(char *key_name, int offset, char *key_buf);
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base);

#endif

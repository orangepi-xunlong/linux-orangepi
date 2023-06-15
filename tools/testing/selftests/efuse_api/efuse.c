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
#include <linux/types.h>

#include "efuse_ioctl.h"

static int efuse_use_ioctl;

int char_to_hex(char input)
{
	if (input >= 'a' && input <= 'f')
		return (input - 'a' + 10);
	else if (input >= 'A' && input <= 'F')
		return (input - 'A' + 10);
	else if (input >= '0' && input <= '9')
		return (input - '0');
	return -1;
}

int string_to_hex(char *input, char *output)
{
	int i, j, value;
	for (i = 0, j = 0; input[i] != 0; i++) {
		value = char_to_hex(input[i]);
		if (value == -1) {
			return -1;
		}
		if (i % 2 == 0) {
			output[j] = value;
		} else {
			output[j] = ((output[j] << 4) & 0xff) + value;
			j++;
		}
	}
	return 0;
}

static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base)
{
	if (*base == 0) {
		if (s[0] == '0') {
			if (tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
			else
				*base = 8;
		} else
			*base = 10;
	}
	if (*base == 16 && s[0] == '0' && tolower(s[1]) == 'x')
		s += 2;
	return s;
}

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0;
	unsigned long value;

	cp = _parse_integer_fixup_radix(cp, &base);

	while (isxdigit(*cp) &&
	       (value = isdigit(*cp) ? *cp - '0' :
					     (islower(*cp) ? toupper(*cp) : *cp) -
					       'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}

	if (endp)
		*endp = (char *)cp;

	return result;
}

static void __is_use_efuse_ctl(void)
{
	int ret;
	if (!efuse_use_ioctl) {
		ret = access(EFUSE_IOCTL_PATH, F_OK);
		if (ret < 0) {
			efuse_use_ioctl = 0;
			return;
		}
		efuse_use_ioctl = 1;
	}
	printf("use efuse ioctl===efuse_use_ioctl=%d\n", efuse_use_ioctl);
}

int sunxi_efuse_read(char *key_name, int offset, int len, char *key_buf)
{
	int fd;
	int ret;
	sunxi_efuse_key_info_t *key_info;
	__is_use_efuse_ctl();

	if (efuse_use_ioctl) {
		fd = open(EFUSE_IOCTL_PATH, O_RDONLY);
		if (fd < 0) {
			printf("Failed to open %s: %s", EFUSE_IOCTL_PATH,
			       strerror(errno));
			return -1;
		}

		key_info = (sunxi_efuse_key_info_t *)malloc(sizeof(*key_info));
		if (!key_info) {
			printf("Out of memory\n");
			close(fd);
			return -1;
		}
		memset(key_info, 0, sizeof(*key_info));

		memcpy(key_info->name, key_name, strlen(key_name));
		key_info->key_data = (uint64_t)key_buf;
		key_info->offset = offset;
		key_info->len = len;
		ret = ioctl(fd, EFUSE_READ, key_info);
		if (ret < 0) {
			printf("efuse read fail: ret=%d %s\n", ret,
			       strerror(errno));
			free(key_info);
			close(fd);
			return -1;
		}
		fsync(fd);
		ret = key_info->len;
		free(key_info);
		close(fd);
		return ret;
	}
	printf("efuse_use_ioctl read fail\n");
	return -1;
}

int sunxi_efuse_write(char *key_name, int offset, char *key_buf)
{
	int fd;
	int ret;
	char out_buf[128];
	sunxi_efuse_key_info_t *key_info;
	__is_use_efuse_ctl();
	//	printf("=====YTR====%s\n",__func__);
	if (efuse_use_ioctl) {
		fd = open(EFUSE_IOCTL_PATH, O_RDONLY);
		//		printf("=====YTR===fd=%d=%s\n",fd,__func__);
		if (fd < 0) {
			printf("Failed to open %s: %s", EFUSE_IOCTL_PATH,
			       strerror(errno));
			return -1;
		}

		key_info = (sunxi_efuse_key_info_t *)malloc(sizeof(*key_info));
		if (!key_info) {
			printf("Out of memory\n");
			close(fd);
			return -1;
		}
		memset(out_buf, 0, sizeof(out_buf));
		if (string_to_hex(key_buf, out_buf) < 0) {
			printf("illegal value !!!\n");
			free(key_info);
			close(fd);
			return -1;
		}
		memset(key_info, 0, sizeof(*key_info));
		memcpy(key_info->name, key_name, strlen(key_name));
		key_info->key_data = (uint64_t)out_buf;
		key_info->len = strlen(key_buf) / 2 + strlen(key_buf) % 2;
		key_info->offset = offset;
		ret = ioctl(fd, EFUSE_WRITE, key_info);

		//		printf("=====YTR=EFUSE_WRITE==ret=%d=%s\n",ret,__func__);
		if (ret < 0) {
			printf("efuse write fail: ret=%d %s\n", ret,
			       strerror(errno));
			free(key_info);
			close(fd);
			return -1;
		}
		fsync(fd);
		free(key_info);
		close(fd);
		return 0;
	}
	printf("efuse_use_ioctl write fail\n");
	return -1;
}

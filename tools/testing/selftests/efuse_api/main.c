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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "efuse_ioctl.h"

void help(void)
{
	printf("-ew <name> <offset> <data>: write key to efuse\n");
	printf("-er <name> <offset> <read_len>: read key from efuse\n");
	printf("-h  : help\n");
}

int main(int argc, char **argv)
{
	char buf[256];
	int ret;

	if (argc < 2) {
		goto error;
	}
	if (!strncmp(argv[1], "-er", 3)) {
		if (argv[4]) {
			if (sunxi_efuse_read(argv[2],
					     simple_strtoul(argv[3], NULL, 16),
					     simple_strtoul(argv[4], NULL, 16),
					     buf) < 0) {
				printf("read efuse data fail\n");
				goto error;
			}
			printf("api buf:\n");
			for (ret = 0; ret < simple_strtoul(argv[4], NULL, 16);
			     ret++)
				printf("%x ", buf[ret]);
			printf("\n");
		}
	} else if (!strncmp(argv[1], "-ew", 3)) {
		if (argv[4]) {
			if (sunxi_efuse_write(argv[2],
					      simple_strtoul(argv[3], NULL, 16),
					      argv[4]) < 0) {
				printf("write efuse data fail\n");
				goto error;
			}
		}
	} else if (!strncmp(argv[1], "-h", 2)) {
		help();
	}
	return 0;

error:
	printf("-----------------------\n");
	printf("----  Test fail  ------\n");
	printf("----  Test fail  ------\n");
	printf("----  Test fail  ------\n");
	printf("-----------------------\n");

	return -1;
}

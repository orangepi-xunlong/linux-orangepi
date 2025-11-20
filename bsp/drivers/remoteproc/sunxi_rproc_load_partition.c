/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <linux/io.h>

#define BUFFER_SIZE      1024

#if defined(CONFIG_AW_REMOTEPROC_UBI_PATH_TRANSFER)
static int transfer_ubi_block_path(struct file *filp, char *block_file_path)
{
	int i = 0;
	char buff[16] = {0};
	char path[64] = {0};

	if (!filp || !block_file_path)
		return -EINVAL;

	memset(path, 0, sizeof(path));
	memset(buff, 0, sizeof(buff));

	scnprintf(buff, sizeof(buff) - 1, "%pD4", filp);

	if (!strstr(buff, "ubi"))
		/*return and no need to transfer*/
		return 1;

	/*path transfer*/
	for (i = 0; i < sizeof(buff); i++)
		if (buff[i] == 'i')
			break;

	if (i >= sizeof(buff) - 1)
		return -EPERM;

	scnprintf(path, sizeof(path) - 1, "/dev/ubiblock%s", &buff[i + 1]);
	memcpy(block_file_path, path, sizeof(path));
	/*transfer path success*/
	return 0;
}
#endif

static int load_from_file(const char *path, void *dst, size_t size)
{
	struct file *filp;
	int ret, bytes = 0;
#if defined(CONFIG_AW_REMOTEPROC_UBI_PATH_TRANSFER)
	char ubi_block_path[64] = {0};
#endif
	if (!path || !dst || !size)
		return -EINVAL;

	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp))
		return PTR_ERR(filp);

#if defined(CONFIG_AW_REMOTEPROC_UBI_PATH_TRANSFER)
	ret = transfer_ubi_block_path(filp, ubi_block_path);
	if (ret < 0)
		goto err_out;

	if (ret == 0) {
		/*reopen new path*/
		filp_close(filp, NULL);
		pr_info("transfer %s to %s\n", path, ubi_block_path);
		filp = filp_open(ubi_block_path, O_RDONLY, 0);
		if (IS_ERR(filp)) {
			pr_err("failed to open %s\n", ubi_block_path);
			return PTR_ERR(filp);
		}
	}
#endif

	while (bytes < size) {
		ret = kernel_read(filp, dst + bytes, size - bytes, &filp->f_pos);
		if (ret < 0)
			goto err_out;
		else if (ret > 0)
			bytes += ret;
		else
			break; // success ?
	}

	filp_close(filp, NULL);
	return bytes;
err_out:
	filp_close(filp, NULL);
	memset(dst, 0, size);
	return ret;
}

int load_from_partition(const char *partition, void *dst, size_t size)
{
	char path[64];

	memset(path, 0, sizeof(path));
	scnprintf(path, sizeof(path) - 1, "/dev/by-name/%s", partition);

	return load_from_file(path, dst, size);
}
EXPORT_SYMBOL(load_from_partition);


// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */

#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/file.h>
#include "rdr_print.h"
#include <linux/user_namespace.h>
#include <asm/current.h>
#include <linux/mm.h>
#include "rdr_utils.h"
#include "rdr_inner.h"
#include <linux/soc/cix/rdr_pub.h>

int rdr_wait_partition(const char *path, int timeouts)
{
	struct kstat m_stat;
	int timeo;

	BB_PRINT_START();
	if (path == NULL) {
		BB_PRINT_ERR("invalid  parameter path\n");
		BB_PRINT_END();
		return -1;
	}

	for (;;) {
		if (rdr_get_suspend_state()) {
			BB_PRINT_PN("%s: wait for suspend\n", __func__);
			msleep(WAIT_TIME);
		} else if (rdr_get_reboot_state()) {
			BB_PRINT_PN("%s: wait for reboot\n", __func__);
			msleep(WAIT_TIME);
		} else {
			break;
		}
	}

	timeo = timeouts;

	while (rdr_vfs_stat(path, &m_stat) != 0) {
		set_current_state(TASK_INTERRUPTIBLE);
		(void)schedule_timeout(HZ / 10);    /* wait for 1/10 second */
		BB_PRINT_DBG("%s,%d: path=%s\n", __func__, __LINE__, path);
		if (timeouts-- < 0) {
			BB_PRINT_ERR("%d:rdr:wait partiton[%s] fail. use [%d]'s . skip!\n",
				__LINE__, path, timeo);
			BB_PRINT_END();
			return -1;
		}
	}

	BB_PRINT_END();
	return 0;
}

/*
 * return:
 *     unsigned int: edition information
 *     0x01          EDITION_USER
 *     0x02          EDITION_INTERNAL_BETA
 *     0x03          EDITION_OVERSEA_BETA
 */
unsigned int rdr_check_edition(void)
{
	char tmp;
	unsigned int type;
	long cnt;
    struct file *file;
    loff_t pos = 0;

    file = filp_open(FILE_EDITION, O_RDONLY, 0);
    if (IS_ERR(file)) {
        return EDITION_USER;
    }

    cnt = kernel_read(file, &tmp, sizeof(tmp), &pos);
    filp_close(file, NULL);

	if (cnt < 0) {
		BB_PRINT_ERR("[%s]: read %s failed, return [%ld]\n",
			__func__, FILE_EDITION, cnt);
		return EDITION_USER;
	}

	if (tmp >= START_CHAR_0 && tmp <= END_CHAR_9) {
		type = (unsigned int)(unsigned char)(tmp - START_CHAR_0);

		if (type == OVERSEA_USER) {
			BB_PRINT_PN("%s: The edition is Oversea BETA, type is %#x\n", __func__, type);
			return EDITION_OVERSEA_BETA;
		} else if (type == BETA_USER) {
			BB_PRINT_PN("%s: The edition is Internal BETA, type is %#x\n", __func__, type);
			return EDITION_INTERNAL_BETA;
		} else if (type == COMMERCIAL_USER) {
			BB_PRINT_PN("%s: The edition is Commercial User, type is %#x\n", __func__, type);
			return EDITION_USER;
		} else {
			BB_PRINT_PN("%s: The edition is default User, type is %#x\n", __func__, type);
			return EDITION_USER;
		}
	} else {
		BB_PRINT_ERR("%s: The edition is default User, please check %s\n", __func__, FILE_EDITION);
		return EDITION_USER;
	}
}
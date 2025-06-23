/*
 * rdr_dump_core.c
 *
 * blackbox. (kernel run data recorder.)
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/ctype.h>
#include <linux/printk.h>
#include <linux/soc/cix/rdr_pub.h>
#include "rdr_inner.h"
#include "rdr_print.h"
#include "rdr_field.h"
#include "rdr_utils.h"
#include <linux/soc/cix/util.h>
#include <asm/cacheflush.h>
#include <linux/kernel.h>
#include <linux/zlib.h>
#include <linux/crc32.h>

/*
 * func description:
 *  save exce info to history.log
 * return
 *  !0   fail
 *  == 0 success
 */
int rdr_save_history_log(struct rdr_exception_info_s *p, char *date, u32 datelen,
				bool is_save_done, u32 bootup_keypoint)
{
	int ret = 0;

	char buf[HISTORY_LOG_SIZE];
	struct kstat historylog_stat;
	char local_path[PATH_MAXLEN];
	char *reboot_from_ap = NULL;
	char *subtype_name = NULL;

	if (p == NULL || date == NULL) {
		BB_PRINT_ERR("%s():%d:invalid parameter, p_exce_info or date is null\n",
			__func__, __LINE__);
		return -1;
	}
	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return 0;
	BB_PRINT_START();
	if (datelen < (strlen(date) + 1))
		date[DATATIME_MAXLEN - 1] = '\0';
	memset(buf, 0, HISTORY_LOG_SIZE);

	if (p->e_reset_core_mask & RDR_AP)
		reboot_from_ap = "true";
	else
		reboot_from_ap = "false";
	/*
	 * The record is normal if in simple reset process.
	 * Otherwise, the string of last_save_not_done needs to be added.
	 */
	subtype_name = rdr_get_subtype_name(p->e_exce_type, p->e_exce_subtype);

	if (is_save_done) {
		if (subtype_name != NULL)
			(void)snprintf(buf, HISTORY_LOG_SIZE,
				"system exception core [%s], reason [%s:%s], time [%s], sysreboot [%s], bootup_keypoint [%u], category [%s]\n",
				rdr_get_exception_core(p->e_from_core),
				rdr_get_exception_type(p->e_exce_type),
				subtype_name,
				date,
				reboot_from_ap,
				bootup_keypoint,
				rdr_get_category_name(p->e_exce_type, p->e_exce_subtype));
		else
			(void)snprintf(buf, HISTORY_LOG_SIZE,
				"system exception core [%s], reason [%s], time [%s], sysreboot [%s], bootup_keypoint [%u], category [%s]\n",
				rdr_get_exception_core(p->e_from_core),
				rdr_get_exception_type(p->e_exce_type),
				date,
				reboot_from_ap,
				bootup_keypoint,
				rdr_get_category_name(p->e_exce_type, p->e_exce_subtype));
	} else {
		if (subtype_name != NULL)
			(void)snprintf(buf, HISTORY_LOG_SIZE,
				"system exception core [%s], reason [%s:%s], time [%s][last_save_not_done], sysreboot [%s], bootup_keypoint [%u], category [%s]\n",
				rdr_get_exception_core(p->e_from_core),
				rdr_get_exception_type(p->e_exce_type),
				subtype_name,
				date,
				reboot_from_ap,
				bootup_keypoint,
				rdr_get_category_name(p->e_exce_type, p->e_exce_subtype));
		else
			(void)snprintf(buf, HISTORY_LOG_SIZE,
				"system exception core [%s], reason [%s], time [%s][last_save_not_done], sysreboot [%s], bootup_keypoint [%u], category [%s]\n",
				rdr_get_exception_core(p->e_from_core),
				rdr_get_exception_type(p->e_exce_type),
				date,
				reboot_from_ap,
				bootup_keypoint,
				rdr_get_category_name(p->e_exce_type, p->e_exce_subtype));
	}

	memset(local_path, 0, PATH_MAXLEN);
	(void)snprintf(local_path, PATH_MAXLEN, "%s/%s", PATH_ROOT, "history.log");

	if (rdr_vfs_stat(local_path, &historylog_stat) == 0 && historylog_stat.size > HISTORY_LOG_MAX)
		rdr_rm_file(local_path); /* delete history.log */

	if (rdr_vfs_stat(PATH_ROOT, &historylog_stat) != 0) {
		ret = rdr_dump_init();
		if (ret) {
			BB_PRINT_ERR("%s():rdr_create_dir fail\n", __func__);
			return ret;
		}
	}

	(void)rdr_savebuf2fs(PATH_ROOT, "history.log", buf, strlen(buf), 1);

	BB_PRINT_END();
	return ret;
}

int rdr_save_history_log_for_undef_exception(struct rdr_syserr_param_s *p)
{
	char buf[HISTORY_LOG_SIZE];
	struct kstat historylog_stat;
	char local_path[PATH_MAXLEN];

	if (p == NULL) {
		BB_PRINT_ERR("exception: NULL\n");
		return -1;
	}

	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return 0;

	BB_PRINT_START();
	memset(buf, 0, HISTORY_LOG_SIZE);
	(void)snprintf(buf, HISTORY_LOG_SIZE,
		"system exception undef. modid[0x%x], arg [0x%x], arg [0x%x]\n",
		p->modid, p->arg1, p->arg2);

	memset(local_path, 0, PATH_MAXLEN);
	(void)snprintf(local_path, PATH_MAXLEN, "%s/%s", PATH_ROOT, "history.log");

	if (rdr_vfs_stat(local_path, &historylog_stat) == 0 &&
	    historylog_stat.blksize > HISTORY_LOG_MAX)
		rdr_rm_file(local_path); /* delete history.log */

	(void)rdr_savebuf2fs(PATH_ROOT, "history.log", buf, strlen(buf), 1);

	BB_PRINT_END();
	return 0;
}


static void rdr_save_logbuf_log(const char *path)
{
	char *src = NULL;
	char *dst = NULL;
	u32 lenth;
	int ret;

	lenth = log_buf_len_get();
	if (lenth == 0) {
		BB_PRINT_ERR("%s():%d:len is zero\n", __func__, __LINE__);
		return;
	}
	src = log_buf_addr_get();
	if (src == NULL) {
		BB_PRINT_ERR("%s():%d:src is null\n", __func__, __LINE__);
		return;
	}

	/* for logbuf write, save before */
	dst = vmalloc(lenth);
	if (dst == NULL) {
		BB_PRINT_ERR("%s():%d:vmalloc buf fail\n", __func__, __LINE__);
		return;
	}

	memcpy(dst, src, lenth - 1);

	ret = rdr_savebuf2fs(path, "logbuf.bin", dst, lenth, 0);
	if (ret < 0) {
		BB_PRINT_ERR("%s(): rdr_savelogbuf2fs error = %d\n", __func__, ret);
		goto err;
	}

err:
	vfree(dst);
	return;
}

void rdr_save_pstore_log(const struct rdr_exception_info_s *p_exce_info, const char *path)
{
	u32 save_flags;

	if (p_exce_info == NULL || path == NULL) {
		BB_PRINT_ERR("%s(): invalid parameter. p_exce_info or path is null\n", __func__);
		return;
	}

	/* system(ap) reset, save logs in reboot */
	if (p_exce_info->e_reset_core_mask & RDR_AP) {
		BB_PRINT_PN("%s(): system reset, no need to save\n", __func__);
		return;
	}

	save_flags = p_exce_info->e_save_log_flags;
	if (save_flags & RDR_SAVE_BL31_LOG) {
		BB_PRINT_PN("%s(): bl31_log saving\n", __func__);
	}

	if (save_flags & RDR_SAVE_DMESG)
		BB_PRINT_PN("%s(): dmsg saving\n", __func__);

	if (save_flags & RDR_SAVE_CONSOLE_MSG)
		BB_PRINT_PN("%s(): console msg saving\n", __func__);

	if (save_flags & RDR_SAVE_LOGBUF) {
		BB_PRINT_PN("%s(): %s logbuf msg saving\n", __func__, path);
		rdr_save_logbuf_log(path);
	}

	return;
}

void rdr_save_cur_baseinfo(const char *logpath)
{
	BB_PRINT_START();
	if (logpath == NULL) {
		BB_PRINT_ERR("logpath is null");
		BB_PRINT_END();
		return;
	}
	/* save pbb to fs */
	(void)rdr_savebuf2fs_compressed(logpath, RDR_BIN, rdr_get_pbb(), rdr_get_pbb_size());

	BB_PRINT_END();
}

void rdr_save_last_baseinfo(const char *logpath)
{
	BB_PRINT_START();
	if (logpath == NULL) {
		BB_PRINT_ERR("logpath is null");
		BB_PRINT_END();
		return;
	}
	/* save pbb to fs */
	(void)rdr_savebuf2fs_compressed(logpath, RDX_BIN, rdr_get_tmppbb(), rdr_get_pbb_size());

	BB_PRINT_END();
}

void rdr_save_ramlog(const char *logpath)
{
	char *paddr;
	u64 size;

	BB_PRINT_START();
	if (logpath == NULL) {
		BB_PRINT_ERR("logpath is null");
		BB_PRINT_END();
		return;
	}

	paddr = rdr_ramlog_mem().vaddr;
	size = rdr_ramlog_mem().size;

	dcache_inval_poc((unsigned long)paddr, (unsigned long)(paddr + size - 1));
	/* save ramlog to fs */
	(void)rdr_savebuf2fs(logpath, RDR_RAMLOG_BIN, paddr, size, 0);

	BB_PRINT_END();
}
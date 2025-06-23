/*
 * rdr_logmonitor_core.c
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
#include <linux/io.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/namei.h>
#include <linux/soc/cix/util.h>
#include <linux/soc/cix/rdr_pub.h>
#include "rdr_inner.h"
#include "rdr_field.h"
#include "rdr_print.h"
#include "rdr_utils.h"

#define DATAREADY_NAME "data-ready"
#define DATA_READY 1
#define DATA_NOT_READY 0

static unsigned int g_dataready_flag = DATA_NOT_READY;
static char *g_history_log_buf = NULL;
static u32 g_history_log_size;
static char g_reboot_logpath[PATH_MAXLEN];

struct getdents_callback {
	struct dir_context ctx;
	const char* path;
	u32 size;
	bool recursion;
};

/*
 * this func is used when devices exception reboot, you can get the logpath
 */
char *rdr_get_reboot_logpath(void)
{
	return g_reboot_logpath;
}
EXPORT_SYMBOL(rdr_get_reboot_logpath);

struct linux_dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[1];
};

int rdr_create_epath_bc(char *path, u32 path_len)
{
	char date[DATATIME_MAXLEN];
	int ret;

	BB_PRINT_START();
	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return -1;
	if (path == NULL) {
		BB_PRINT_ERR("invalid  parameter\n");
		BB_PRINT_END();
		return -1;
	}

	memset(date, 0, DATATIME_MAXLEN);

	snprintf(date, DATATIME_MAXLEN, "%s-%08lld", rdr_get_timestamp(), rdr_get_tick());

	if (path_len < PATH_MAXLEN) {
		BB_PRINT_ERR("invalid len\n");
		return -1;
	}

	(void)snprintf(path, PATH_MAXLEN, "%s%s/", PATH_ROOT, date);
	BB_PRINT_PN("date buf error, cur log path:[%s]\n", path);

	ret = rdr_create_dir(path);
	if (ret == 0) {
		memset(g_reboot_logpath, 0, PATH_MAXLEN);
		strncpy(g_reboot_logpath, path, strlen(path));
	}

	BB_PRINT_END();

	return ret;
}

int rdr_create_exception_path(struct rdr_exception_info_s *e, char *path,
				char *date, u32 datelen)
{
	int ret;

	BB_PRINT_START();
	if (!check_himntn(HIMNTN_GOBAL_RESETLOG)) {
		BB_PRINT_DBG("HIMNTN_GOBAL_RESETLOG is disabled ... \n");
		return -1;
	}

	if (e == NULL || path == NULL || date == NULL) {
		BB_PRINT_ERR("invalid  parameter e, path or date\n");
		BB_PRINT_END();
		return ret = -1;
	}

	if (datelen < DATATIME_MAXLEN)
		BB_PRINT_ERR("invalid  parameter datelen\n");

	memset(date, 0, DATATIME_MAXLEN);

	ret = snprintf(date, DATATIME_MAXLEN, "%s-%08lld",
		rdr_get_timestamp(), rdr_get_tick());
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
		return ret;
	}

	memset(path, 0, PATH_MAXLEN);
	ret = snprintf(path, PATH_MAXLEN, "%s/%s/", PATH_ROOT, date);
	if (unlikely(ret < 0)) {
		BB_PRINT_ERR("[%s], snprintf_s ret %d!\n", __func__, ret);
		return ret;
	}

	ret = rdr_create_dir(path);
	BB_PRINT_END();
	return ret;
}

int bbox_create_dfxlog_path(char *path, char *date, u32 data_len)
{
	int ret;
	static int number = 1;

	BB_PRINT_START();
	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return -1;

	if (path == NULL || date == NULL) {
		BB_PRINT_ERR("invalid  parameter\n");
		BB_PRINT_END();
		return -1;
	}

	if (data_len < DATATIME_MAXLEN)
		BB_PRINT_ERR("invalid  len\n");

	memset(date, 0, DATATIME_MAXLEN);

	(void)snprintf(date, DATATIME_MAXLEN, "%s-%08lld",
		 rdr_get_timestamp(), rdr_get_tick() + number);

	number++;
	memset(path, 0, PATH_MAXLEN);
	(void)snprintf(path, PATH_MAXLEN, "%s%s/", PATH_ROOT, date);

	ret = rdr_create_dir(path);
	BB_PRINT_END();
	return ret;
}

static LIST_HEAD(__rdr_logpath_info_list);
static DEFINE_MUTEX(__rdr_logpath_info_list_mutex);
struct logpath_info_s {
	struct list_head s_list;
	struct timespec64 ctime;
	u32 logpath_idx;
	char path[PATH_MAXLEN];
};


struct rdr_log_tm_range {
	char name[LOG_TIME_MAXLEN];
	int idx[2]; /* start, len */
	int rg[2]; /* min, max */
};

const struct rdr_log_tm_range tbl_rdr_logdir_tm_rg[] = {
	/* name  idx{start,len}  rg{min,max} */
	{ "year", { 0, 4 }, { 1900, 2200 } },
	{ "month", { 4, 2 }, { 1, 12 } },
	{ "day", { 6, 2 }, { 1, 31 } },
	{ "hour", { 8, 2 }, { 0, 23 } },
	{ "minute", { 10, 2 }, { 0, 59 } },
	{ "second", { 12, 2 }, { 0, 59 } },
};

static int rdr_is_logdir_nm_tm(const char *buf, int len)
{
	int i;
	char tempstr[LOG_TIME_MAXLEN];
	int val;
	const struct rdr_log_tm_range *pt_rg = NULL;

	if (buf == NULL) {
		BB_PRINT_ERR("%s():%d:invalid  parameter buf!\n", __func__, __LINE__);
		return 0;
	}

	pt_rg = (const struct rdr_log_tm_range *)&tbl_rdr_logdir_tm_rg;

	/* Judge if all char is number */
	for (i = 0; i < len; i++) {
		if (buf[i] > '9' || buf[i] < '0')
			return 0;
	}

	/* Judge all field range */
	for (i = 0; (unsigned int)i < ARRAY_SIZE(tbl_rdr_logdir_tm_rg); i++, pt_rg++) {
		memcpy(tempstr, buf + pt_rg->idx[0], pt_rg->idx[1]);
		tempstr[pt_rg->idx[1]] = 0;
		/* cppcheck-suppress * */
		if (sscanf(tempstr, "%d", &val) != 1) {
			BB_PRINT_ERR("[%s], val get failed!\n", __func__);
			return 0;
		}
		BB_PRINT_DBG("%s, val = %d, <%d,%d>\n", pt_rg->name, val,
			pt_rg->rg[0], pt_rg->rg[1]);
		if (val < pt_rg->rg[0] || val > pt_rg->rg[1])
			return 0;
	}

	return 1;
}

u64 rdr_cal_tm_from_logdir_name(const char *path)
{
	char sec[DATA_MAXLEN + 1];
	static u64 date_sec; /* default value is 0 */

	if (path == NULL) {
		BB_PRINT_ERR("%s():%d:invalid  parameter path!\n", __func__, __LINE__);
		return 0;
	}
	strncpy(sec, path, DATA_MAXLEN);
	sec[DATA_MAXLEN] = 0;
	if (rdr_is_logdir_nm_tm(sec, DATA_MAXLEN)) {
		if (sscanf(sec, "%lld", &date_sec) != 1)
			BB_PRINT_ERR("[%s], date_sec get failed!\n", __func__);
	} else {
		date_sec++; /* when dir name error, it is a little bigger than perv */
	}
	BB_PRINT_DBG("[%s], date_sec = %lld\n", __func__, date_sec);
	return date_sec;
}

static int rdr_get_history_log_buffer(char **buffer, u32 *size)
{
	if (!g_history_log_buf || !g_history_log_size) {
		BB_PRINT_ERR("[%s]g_history_log_buf not update\n", __func__);
		return -1;
	}

	*buffer = g_history_log_buf;
	*size = g_history_log_size;
	return 0;
}

static void rdr_update_history_log_buffer(void)
{
	char path[DST_TMP_PATH_MAX_LEN];
	struct kstat stat;
	ssize_t cnt;
	struct file *file;
	loff_t pos = 0;
	int ret;

	kfree(g_history_log_buf);
	g_history_log_buf = NULL;
	g_history_log_size = 0;

	memset(path, 0x0, DST_TMP_PATH_MAX_LEN);
	ret = snprintf(path, DST_TMP_PATH_MAX_LEN, "%s/%s", PATH_ROOT, "history.log");
	if (ret < 0) {
		BB_PRINT_ERR("[%s]snprintf_s history.log error\n", __func__);
		return;
	}

	file = filp_open(path, O_RDONLY, FILE_LIMIT);
	if (IS_ERR(file)) {
		BB_PRINT_ERR("[%s]history.log open failed \n", __func__);
		return;
	}

	memset(&stat, 0x0, sizeof(stat));
	ret = rdr_vfs_stat(path, &stat);
	if (ret) {
		BB_PRINT_ERR("[%s]get stat error, ret = %d\n", __func__, ret);
		goto err_close;
	}

	if (stat.size > HISTORY_LOG_MAX)
		stat.size = HISTORY_LOG_MAX;

	g_history_log_buf = kzalloc(stat.size + 1, GFP_KERNEL);
	if (g_history_log_buf == NULL) {
		BB_PRINT_ERR("[%s]kzalloc g_history_log_buf error\n", __func__);
		goto err_close;
	}

	cnt = kernel_read(file, g_history_log_buf, stat.size, &pos);
	if (cnt <= 0) {
		BB_PRINT_ERR("[%s]read_read error, cnt = %ld\n", __func__, cnt);
		goto err_free;
	}
	g_history_log_size = cnt;

    filp_close(file, NULL);
	return;

err_free:
	kfree(g_history_log_buf);
	g_history_log_buf = NULL;
err_close:
	filp_close(file, NULL);
}

static u32 rdr_count_logpath_idx(const char *filename)
{
	char date_str[DATA_MAXLEN + 1];
	char *buf = NULL;
	char *end_buf = NULL;
	u32 size, min_size_to_search;
	u32 idx = 1;
	int ret;

	ret = rdr_get_history_log_buffer(&buf, &size);
	if (ret < 0) {
		BB_PRINT_ERR("[%s]rdr_get_history_log_buffer error\n", __func__);
		return 0;
	}
	end_buf = buf + size;

	if (strlen(filename) > DATATIME_MAXLEN) {
		BB_PRINT_ERR("[%s]path:%s is too long \n", __func__, filename);
		return 0;
	}
	memset(date_str, 0, DATA_MAXLEN + 1);
	memcpy(date_str, filename, strlen(filename));

	while (buf < end_buf) {
		min_size_to_search = (end_buf - buf) > HISTORY_LOG_SIZE ? HISTORY_LOG_SIZE : (end_buf - buf);
		buf = strnstr(buf, "time [", min_size_to_search);
		if (buf == NULL)
			break;
		buf = buf + sizeof("time [") - 1;
		BB_PRINT_DBG("%s,%d: buf=%s, date_str=%s \n", __func__, __LINE__, buf, date_str);
		if (!strncmp(buf, date_str, DATA_MAXLEN))
			return idx;
		idx++;
	}
	return 0;
}

/*
 * Description:    Maybe a logpath is in list, but is not exist.
 *                 So need empty logpath_list.
 */
static void rdr_empty_logpath_list(void)
{
	struct logpath_info_s *p_info = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;

	BB_PRINT_START();
	list_for_each_safe(cur, next, &__rdr_logpath_info_list) {
		p_info = list_entry(cur, struct logpath_info_s, s_list);
		if (p_info == NULL) {
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n",
				__func__, __LINE__);
			continue;
		}
		list_del(cur);
		kfree(p_info);
	}

	BB_PRINT_END();
	return;
}

static void rdr_check_logpath_repeat(const struct logpath_info_s *info)
{
	struct logpath_info_s *p_info = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;

	if (info == NULL) {
		BB_PRINT_ERR("%s():%d:invalid  parameter info\n", __func__, __LINE__);
		return;
	}

	list_for_each_safe(cur, next, &__rdr_logpath_info_list) {
		p_info = list_entry(cur, struct logpath_info_s, s_list);
		if (p_info == NULL) {
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n", __func__, __LINE__);
			continue;
		}
		if (memcmp(info->path, p_info->path, strlen(info->path)) == 0) {
			list_del(cur);
			kfree(p_info);
		}
	}
}

static u32 __rdr_add_logpath_list(struct logpath_info_s *info)
{
	struct logpath_info_s *p_info = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;

	if (list_empty(&__rdr_logpath_info_list)) {
		list_add_tail(&info->s_list, &__rdr_logpath_info_list);
		BB_PRINT_END();
		goto out;
	}
	p_info = list_entry(__rdr_logpath_info_list.next,
		struct logpath_info_s, s_list);
	if (info->logpath_idx >= p_info->logpath_idx) {
		list_add(&info->s_list, &__rdr_logpath_info_list);
		goto out;
	}
	list_for_each_safe(cur, next, &__rdr_logpath_info_list) {
		p_info = list_entry(cur, struct logpath_info_s, s_list);
		if (p_info == NULL) {
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n",
				__func__, __LINE__);
			continue;
		}
		if (memcmp(info->path, p_info->path, strlen(info->path)) == 0) {
			p_info->ctime.tv_sec = info->ctime.tv_sec;
			p_info->ctime.tv_nsec = info->ctime.tv_nsec;
		}
		if (info->logpath_idx >= p_info->logpath_idx) {
			list_add_tail(&info->s_list, cur);
			goto out;
		}
	}
	list_add_tail(&info->s_list, &__rdr_logpath_info_list);
out:
	return 0;
}

static void rdr_add_logpath_list(const char *path, const char* filename, struct timespec64 *time)
{
	struct logpath_info_s *lp_info = NULL;
	u32 len;

	lp_info = kmalloc(sizeof(*lp_info), GFP_ATOMIC);
	if (lp_info == NULL) {
		BB_PRINT_ERR("kmalloc logpath_info_s faild\n");
		return;
	}

	memset(lp_info, 0, sizeof(*lp_info));
	len = strlen(path);
	if (len >= sizeof(lp_info->path)) {
		BB_PRINT_ERR("%s():%d:memcpy fail!\n", __func__, __LINE__);
		kfree(lp_info);
		return;
	}
	memcpy(lp_info->path, path, len);
	lp_info->ctime.tv_sec = time->tv_sec;
	lp_info->ctime.tv_nsec = time->tv_nsec;
	lp_info->logpath_idx = rdr_count_logpath_idx(filename);
	BB_PRINT_DBG("%s,%d: %s : %d\n", __func__, __LINE__, filename, lp_info->logpath_idx);

	rdr_check_logpath_repeat(lp_info);
	__rdr_add_logpath_list(lp_info);
}

static void rdr_print_all_logpath(void)
{
	int index = 1;
	struct logpath_info_s *p_module_ops = NULL;
	struct list_head *cur = NULL;
	struct list_head *next = NULL;

	BB_PRINT_START();
	list_for_each_safe(cur, next, &__rdr_logpath_info_list) {
		p_module_ops = list_entry(cur, struct logpath_info_s, s_list);
		if (p_module_ops == NULL) {
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n",
				__func__, __LINE__);
			continue;
		}
		BB_PRINT_PN("==========[%.2d]-start==========\n", index);
		BB_PRINT_PN(" path:    [%s]\n", p_module_ops->path);
		BB_PRINT_PN(" ctime:   [0x%llx]\n", (u64) (p_module_ops->ctime.tv_sec));
		BB_PRINT_PN("==========[%.2d]-e n d==========\n", index);
		index++;
	}

	BB_PRINT_END();
}

char *known[] = {
	"",
};

char *ignore[] = {
	".",
	"..",
	PATH_MEMDUMP,
	"running_trace",
	"history.log",
	"reboot_times.log",
	"modem_log",
};

int rdr_dump_init()
{
	int ret;

	while (rdr_wait_partition(PATH_MNTN_PARTITION, RDR_TIME_OUT) != 0);

	/* check and set version info */
	(void)rdr_check_edition();

	ret = rdr_create_dir(PATH_ROOT);
	if (ret)
		return ret;

	/* according to authority requirements,hisi_logs and its subdir are root-system */
	ret = (int)rdr_chown((const char __user *)PATH_ROOT, ROOT_UID, SYSTEM_GID, true);
	if (ret) {
		BB_PRINT_ERR("[%s], chown %s uid [%d] gid [%d] failed err [%d]!\n",
			__func__, PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
		return ret;
	}

	return 0;
}

void rdr_dump_exit(void)
{
}

static int rdr_check_logpath_legality(const char *path)
{
	int ret = 1; /* 0 => ignore. !0 => process. */
	int size;
	int index;

	size = sizeof(ignore) / sizeof(char *);
	for (index = 0; index < size; index++) {
		if (strncmp(path, ignore[index], strlen(path)) == 0) {
			ret = -1;
			goto out;
		}
	}

	size = DATA_MAXLEN + TIME_MAXLEN + 1;
	for (index = 0; path[index] != '\0' && index < size; index++) {
		if (path[index] < '0' || path[index] > '9') {
			if (index == DATA_MAXLEN && path[index] == '-')
				continue;
			BB_PRINT_PN("invalid path [%s]\n", path);
			BB_PRINT_END();
			ret = 0;
			goto out;
		}
	}
out:
	return ret;
}

static int rdr_dir_size_for_directory(const char* filename, bool recursion,
				u32 *size, struct kstat *stat, char* fullname)
{
	int ret;

	ret = rdr_check_logpath_legality(filename);
	if (ret == -1)
		return -1;

	if (!recursion && ret == 0) {
		BB_PRINT_ERR("check legality: invalid path:%s\n", fullname);
		if (rdr_rm_dir(fullname) < 0)
			BB_PRINT_ERR("%s(): failed to del %s\n", __func__, fullname);
		return -1;
	}

	if (recursion)
		/* cppcheck-suppress */
		(*size) += rdr_dir_size(fullname, PATH_MAXLEN, recursion);
	else
		rdr_add_logpath_list(fullname, filename, &(stat->ctime));

	return 0;
}

// define the actor function
static bool rdr_actor_function(struct dir_context *ctx, const char *name, int len,
			loff_t pos, u64 ino, unsigned int d_type)
{
	struct kstat stat;
	char fullname[PATH_MAXLEN];
	struct getdents_callback *buf =
		container_of(ctx, struct getdents_callback, ctx);

	memset(fullname, 0, PATH_MAXLEN);

	(void)snprintf(fullname, sizeof(fullname), "%s/%s", buf->path, name);

	BB_PRINT_DBG("[%s,%d] path:%s \n", __func__, __LINE__, fullname);
	if (rdr_vfs_stat(fullname, &stat) != 0) {
		BB_PRINT_ERR("[%s,%d] path:%s stat failed \n", __func__, __LINE__, fullname);
		return false;
	}

	if (S_ISDIR(stat.mode) && rdr_dir_size_for_directory(name, buf->recursion, &buf->size, &stat, fullname)) {
		BB_PRINT_DBG("[%s,%d] dir:%s \n", __func__, __LINE__, fullname);
		return true;
	} else if (S_ISREG(stat.mode)) {
		BB_PRINT_DBG("[%s,%d] file:%s \n", __func__, __LINE__, fullname);
		buf->size += stat.size;
	}

	return true;
}

int rdr_dir_size(const char *path, u32 path_len, bool recursion)
{
	/* DT_DIR, DT_REG */
	int ret;
	struct path kpath;
	struct file* file;
	struct getdents_callback buffer = {
		.ctx.actor = rdr_actor_function,
		.size = 0,
		.recursion = 0,
	};

	if (path == NULL) {
		BB_PRINT_ERR("rdr:path is null\n");
		return 0;
	}

	if (path_len > PATH_MAXLEN) {
		BB_PRINT_ERR("rdr:len is not correct\n");
		return 0;
	}

	if (strncmp(path, PATH_ROOT, strlen(PATH_ROOT))) {
		BB_PRINT_ERR("rdr:%s(), path [%s] err\n", __func__, path);
		return 0;
	}

	ret = kern_path(path, 0, &kpath);
	if (ret) {
		printk(KERN_ERR "Failed to get path:%s %d\n", path, ret);
		return 0;
	}

	// open the directory
	file = dentry_open(&kpath, O_RDONLY, current_cred());
	if (IS_ERR(file)) {
		BB_PRINT_ERR("rdr:%s(),open %s fail\n", __func__, path);
		BB_PRINT_END();
		return 0;
	}

	buffer.recursion = recursion;
	buffer.path = path;
	// iterate over the directory entries
	ret = iterate_dir(file, &buffer.ctx);

	// clean up
	fput(file);
	path_put(&kpath);

	if (ret) {
		BB_PRINT_ERR("rdr failed to iterate directory: %d\n", ret);
		return 0;
	}

	return buffer.size;
}

static bool rdr_check_log_mark(u32 rdr_max_size, u32 rdr_max_logs)
{
	struct logpath_info_s *p_info = NULL;
	struct rdr_list_head rdr_list_head;
	u32 size = 0;
	u32 tmpsize;
	bool ret = true;
	u32 rdr_log_nums = 0;

	rdr_list_head.cur = NULL;
	rdr_list_head.next = NULL;
	mutex_lock(&__rdr_logpath_info_list_mutex);
	rdr_empty_logpath_list();
	rdr_update_history_log_buffer();
	size += rdr_dir_size(PATH_ROOT, PATH_MAXLEN, false);
	BB_PRINT_DBG("%s,%d, size = %d\n", __func__, __LINE__, size);
	list_for_each_safe(rdr_list_head.cur, rdr_list_head.next, &__rdr_logpath_info_list) {
		p_info = list_entry(rdr_list_head.cur, struct logpath_info_s, s_list);
		if (p_info == NULL) {
			list_del(rdr_list_head.cur);
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n",
				__func__, __LINE__);
			continue;
		}

		tmpsize = rdr_dir_size(p_info->path, PATH_MAXLEN, true);
		if ((tmpsize + size > rdr_max_size) || (++rdr_log_nums > rdr_max_logs)) {
			BB_PRINT_PN("over size: cur[0x%x], next[0x%x],max[0x%x], or over nums:cur[%u], max[%u]\n",
				size, tmpsize, rdr_max_size, rdr_log_nums, rdr_max_logs);
			ret = false;
			break;
		}
		size += tmpsize;
	}
	mutex_unlock(&__rdr_logpath_info_list_mutex);

	return ret;
}

bool rdr_check_log_rights(void)
{
	bool ret = true;
	u32 rdr_logs_low;
	u32 rdr_logs_high;
	u32 rdr_max_size;

	BB_PRINT_START();
	rdr_logs_low = rdr_get_lognum();
	rdr_max_size = rdr_get_logsize();
	BB_PRINT_DBG("%s,%d: rdr_logs_low=%d, rdr_max_size=%d \n", 
		__func__, __LINE__, rdr_logs_low, rdr_max_size);

	ret = rdr_check_log_mark(rdr_max_size, rdr_logs_low);
	if (!ret) {
		BB_PRINT_ERR("%s():bbox timestamp logs are filled, clean thems\n", __func__);
		rdr_count_size();

		rdr_logs_high = rdr_logs_low + LOG_TIME_MAXLEN;
		ret = rdr_check_log_mark(rdr_max_size, rdr_logs_high);
		if (!ret)
			BB_PRINT_ERR("%s():bbox has no rights to save log!\n", __func__);
	}
	BB_PRINT_END();

	return ret;
}

void rdr_count_size(void)
{
	struct logpath_info_s *p_info = NULL;
	struct rdr_list_head rdr_list_head;
	bool oversize = false;
	int ret;
	struct rdr_log_count rdr_log_count;

	BB_PRINT_START();
	rdr_log_count.rdr_max_logs = rdr_get_lognum();
	rdr_list_head.cur = NULL;
	rdr_list_head.next = NULL;
	rdr_log_count.size = 0;
	rdr_log_count.rdr_log_nums = 0;

	mutex_lock(&__rdr_logpath_info_list_mutex);
	rdr_empty_logpath_list();
	rdr_update_history_log_buffer();
	rdr_log_count.size += rdr_dir_size(PATH_ROOT, PATH_MAXLEN, false);
	list_for_each_safe(rdr_list_head.cur, rdr_list_head.next, &__rdr_logpath_info_list) {
		p_info = list_entry(rdr_list_head.cur, struct logpath_info_s, s_list);
		if (p_info == NULL) {
			list_del(rdr_list_head.cur);
			BB_PRINT_ERR("It might be better to look around here. %s:%d\n",
				__func__, __LINE__);
			continue;
		}

		if (oversize) {
			BB_PRINT_PN("over size: cur[0x%x], max[0x%llx]\n",
				rdr_log_count.size, rdr_get_logsize());
			if (rdr_rm_dir(p_info->path) < 0)
				BB_PRINT_ERR("It might be better to look around here. %s:%d\n", __func__, __LINE__);

			list_del(rdr_list_head.cur);
			kfree(p_info);
			p_info = NULL;
			continue;
		}

		/* check the size of dir recursively,if it is oversize,remove followed records */
		rdr_log_count.tmpsize = rdr_dir_size(p_info->path, PATH_MAXLEN, true);
		BB_PRINT_DBG("%s,%d: path=%s size=%d \n", __func__, __LINE__, p_info->path, rdr_log_count.tmpsize);
		
		if ((rdr_log_count.tmpsize + rdr_log_count.size > rdr_get_logsize()) ||
			(++rdr_log_count.rdr_log_nums > rdr_log_count.rdr_max_logs)) {
			oversize = true;
			BB_PRINT_PN("over size: cur[0x%x], next[0x%x],max[0x%llx], or over nums:cur[%u], max[%u]\n",
				rdr_log_count.size, rdr_log_count.tmpsize, rdr_get_logsize(),
				rdr_log_count.rdr_log_nums, rdr_log_count.rdr_max_logs);
			if (rdr_rm_dir(p_info->path) < 0)
				BB_PRINT_ERR("It might be better to look around here. %s:%d\n", __func__, __LINE__);
			list_del(rdr_list_head.cur);
			kfree(p_info);
			p_info = NULL;
		} else {
			rdr_log_count.size += rdr_log_count.tmpsize;
		}
	}
	mutex_unlock(&__rdr_logpath_info_list_mutex);

	/* according to authority requirements,hisi_logs and its subdir are root-system */
	ret = (int)rdr_chown((const char __user *)PATH_ROOT, ROOT_UID, SYSTEM_GID, true);
	if (ret) {
		BB_PRINT_ERR("[%s], chown %s uid [%d] gid [%d] failed err [%d]!\n",
			__func__, PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
		return;
	}

	mutex_lock(&__rdr_logpath_info_list_mutex);
	rdr_print_all_logpath();
	mutex_unlock(&__rdr_logpath_info_list_mutex);
	BB_PRINT_END();
}

/*
 * Description:    show g_dataready_flag
 * Input:          struct seq_file *m, void *v
 * Return:         0:success;other:fail
 */
static int dataready_info_show(struct seq_file *m, void *v)
{
	if (m == NULL) {
		BB_PRINT_ERR("seq_file:m is null\n");
		return -1;
	}

	seq_printf(m, "%u\n", g_dataready_flag);
	return 0;
}

/*
 * Description:    write /proc/data-ready, for get the status of data_partition
 * Input:          file;buffer;count;data
 * Return:         >0:success;other:fail
 */
static ssize_t dataready_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	ssize_t ret = -EINVAL;
	char tmp;

	/* buffer must be '1' or '0', so count<=2 */
	if (count > 2)
		return ret;

	if (!buffer)
		return ret;

	/* should ignore character '\n' */
	if (copy_from_user(&tmp, buffer, sizeof(tmp)))
		return -EFAULT;

	if (tmp == '1')
		g_dataready_flag = DATA_READY;
	else if (tmp == '0')
		g_dataready_flag = DATA_NOT_READY;
	else
		BB_PRINT_ERR("%s():%d:input arg invalid[%c]\n", __func__, __LINE__, tmp);

	return 1;
}

/*
 * Description:    open /proc/data-ready
 * Input:          inode;file
 * Return:         0:success;other:fail
 */
static int dataready_open(struct inode *inode, struct file *file)
{
	if (!file)
		return -EFAULT;

	return single_open(file, dataready_info_show, NULL);
}


static const struct proc_ops dataready_proc_fops = {
	.proc_open       = dataready_open,
	.proc_read       = seq_read,
	.proc_write      = dataready_write_proc,
	.proc_lseek     = seq_lseek,
	.proc_release    = single_release,
};

/*
 * Description:    create /proc/data-ready
 * Return:         0:success;-1:fail
 */
static int __init dataready_proc_init(void)
{
	struct proc_dir_entry *proc_dir_entry;

	proc_dir_entry = proc_create(DATAREADY_NAME,
		FILE_LIMIT,
		NULL,
		&dataready_proc_fops);
	if (!proc_dir_entry) {
		BB_PRINT_ERR("proc_create DATAREADY_NAME fail\n");
		return -1;
	}

	return 0;
}

module_init(dataready_proc_init);

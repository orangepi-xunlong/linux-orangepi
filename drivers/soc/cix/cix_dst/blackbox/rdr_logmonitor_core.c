// SPDX-License-Identifier: GPL-2.0-only
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

#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/sort.h>
#include "rdr_field.h"
#include "rdr_inner.h"
#include "rdr_print.h"

#define LOGPATH_INDEX(is_last) (!(!is_last))

#define LOGDIR_USE BIT(0)
#define LOGDIR_INIT BIT(1)

struct dir_info {
	char name[DATA_MAXLEN + TIME_MAXLEN + 2];
	u64 size;
	u64 date;
	u64 time;
};

struct dir_array {
	char *path;
	struct dir_info *dir;
	u32 dir_num;
	u64 size;
	int err;
};

struct getdents_callback {
	struct dir_context ctx;
	bool recursion;
	bool is_first;
	struct dir_array *dirs;
};

struct date_path_info {
	char path[PATH_MAXLEN];
	char date[DATATIME_MAXLEN];
	atomic_t flag;
};

static char *ignore[] = {
	".",
	"..",
	"history.log",
	"reboot_times.log",

};
static struct date_path_info g_loginfo[2];

static void rdr_logdir_put(struct dir_array *dirs)
{
	if (IS_ERR_OR_NULL(dirs))
		return;
	BB_DBG("dirs:%s, free: %px\n", dirs->path, dirs);
	kfree(dirs->path);
	kfree(dirs->dir);
	kfree(dirs);
}

static struct dir_array *rdr_logdir_init_mem(char *path)
{
	struct dir_array *dirs;

	dirs = kzalloc(sizeof(struct dir_array), GFP_KERNEL);
	if (IS_ERR_OR_NULL(dirs))
		return NULL;

	dirs->path = kzalloc(strlen(path) + 1, GFP_KERNEL);
	if (IS_ERR_OR_NULL(dirs->path)) {
		kfree(dirs);
		return NULL;
	}
	memcpy(dirs->path, path, strlen(path));
	BB_DBG("dirs:%s, malloc: %px\n", dirs->path, dirs);
	return dirs;
}

static bool rdr_logdir_actor(struct dir_context *ctx, const char *name,
			     int namelen, loff_t pos, u64 ino,
			     unsigned int d_type);
static struct dir_array *rdr_logdir_get(const char *path, bool recursion,
					bool is_first)
{
	/* DT_DIR, DT_REG */
	int ret;
	struct path kpath;
	struct file *file;
	struct kstat stat;
	struct getdents_callback buffer = {
		.ctx.actor = rdr_logdir_actor,
		.recursion = recursion,
		.is_first = is_first,
		.dirs = NULL,
	};

	if (path == NULL) {
		BB_ERR("path is null\n");
		return NULL;
	}

	if (!strnstr(path, PATH_ROOT, strlen(PATH_ROOT))) {
		BB_ERR("path is err, %s\n", path);
		return NULL;
	}

	buffer.dirs = rdr_logdir_init_mem((char *)path);
	if (IS_ERR_OR_NULL(buffer.dirs)) {
		BB_ERR("path is err, %s\n", path);
		return NULL;
	}

	memset(&stat, 0, sizeof(stat));
	if (rdr_vfs_stat((char *)path, &stat))
		BB_ERR("path size err, %s\n", path);

	buffer.dirs->size += stat.size;
	ret = kern_path(path, 0, &kpath);
	if (ret) {
		BB_ERR("Failed to get path:%s %d\n", path, ret);
		goto close;
	}

	// open the directory
	file = dentry_open(&kpath, O_RDONLY, current_cred());
	if (IS_ERR(file)) {
		BB_ERR("open %s fail\n", path);
		goto close;
	}

	if (iterate_dir(file, &buffer.ctx))
		BB_ERR("iterate err\n");
	// clean up
	fput(file);

close:
	path_put(&kpath);
	return buffer.dirs;
}

static void rdr_logdir_add(struct dir_array *dirs, char *name, u64 size)
{
	u32 index = 0;
	char *time, *date;

	if (IS_ERR_OR_NULL(dirs))
		return;

	index = dirs->dir_num++;
	dirs->dir = krealloc(dirs->dir,
			     (dirs->dir_num) * sizeof(struct dir_info),
			     GFP_KERNEL);
	if (IS_ERR_OR_NULL(dirs->dir))
		dirs->err = -ENOMEM;

	if (dirs->err)
		return;

	strncpy(dirs->dir[index].name, name, sizeof(dirs->dir[index].name));
	dirs->dir[index].size = size;
	/*get date time*/
	date = dirs->dir[index].name;
	time = strnchr(date, sizeof(dirs->dir[index].name), '-');
	*time = '\0';

	if (kstrtou64(date, 10, &dirs->dir[index].date))
		dirs->dir[index].date = 0;
	if (kstrtou64(time + 1, 10, &dirs->dir[index].time))
		dirs->dir[index].time = 0;
	*time = '-';
}

static void rdr_logdir_show(struct dir_array *dirs)
{
	if (IS_ERR_OR_NULL(dirs))
		return;

	BB_PN("dir:%s, size: %llu\n", dirs->path, dirs->size);
	for (int i = 0; i < dirs->dir_num; i++)
		BB_PN("\t-->dir:%s, size: %llu\n", dirs->dir[i].name,
		      dirs->dir[i].size);
}

static bool rdr_logdir_actor(struct dir_context *ctx, const char *name,
			     int namelen, loff_t pos, u64 ino,
			     unsigned int d_type)
{
	struct kstat stat;
	char *fullname;
	struct getdents_callback *buf =
		container_of(ctx, struct getdents_callback, ctx);
	struct dir_array *dirs = NULL;

	/*check ignore*/
	for (int i = 0; i < ARRAY_SIZE(ignore); i++) {
		if (strncmp(name, ignore[i], strlen(name)) == 0)
			return true;
	}

	fullname = kzalloc(PATH_MAXLEN, GFP_KERNEL);
	if (IS_ERR_OR_NULL(fullname)) {
		BB_ERR("kzalloc failed\n");
		return false;
	}

	(void)snprintf(fullname, PATH_MAXLEN, "%s/%s", buf->dirs->path, name);
	if (rdr_vfs_stat(fullname, &stat)) {
		BB_ERR("path:%s stat failed\n", fullname);
		return false;
	}

	if (d_type == DT_REG) {
		buf->dirs->size += stat.size;
		BB_DBG("%s: %lld, total: %llu\n", fullname, stat.size,
		       buf->dirs->size);
		goto free;
	}

	if (d_type != DT_DIR)
		goto free;
	if (!buf->recursion)
		goto free;
	if (!buf->is_first)
		goto cal_size;

	/*check dir is invalid*/
	for (int i = 0; i < namelen; i++) {
		if (name[i] >= '0' && name[i] <= '9')
			continue;
		if (name[i] == '-')
			continue;
		rdr_rm_dir(fullname);
		goto free;
	}

cal_size:
	dirs = rdr_logdir_get(fullname, buf->recursion, false);
	if (IS_ERR_OR_NULL(dirs)) {
		BB_ERR("get dir info failed\n");
		goto free;
	}
	buf->dirs->size += dirs->size;
	if (buf->is_first)
		rdr_logdir_add(buf->dirs, (char *)name, dirs->size);
	rdr_logdir_put(dirs);

free:
	kfree(fullname);
	return true;
}

static int rdr_logdir_rule_process(struct dir_array *dirs, u64 maxsize,
				   u32 maxnum)
{
	u32 dir_num = dirs->dir_num;
	char *path = kzalloc(PATH_MAXLEN, GFP_KERNEL);
	struct dir_info *dir;

	if (IS_ERR_OR_NULL(path))
		return -1;

	for (u32 i = 0; i < dir_num; i++) {
		if (dirs->size <= maxsize && dirs->dir_num <= maxnum)
			break;
		dir = &dirs->dir[dir_num - i - 1];
		snprintf(path, PATH_MAXLEN, "%s/%s", dirs->path, dir->name);
		rdr_rm_dir(path);
		dirs->size -= dir->size;
		dirs->dir_num--;
	}

	kfree(path);
	return 0;
}

static int rdr_logdir_sort_handle(const void *va, const void *vb)
{
	const struct dir_info *a = va;
	const struct dir_info *b = vb;

	if (a->date < b->date)
		return 1;
	if (a->date > b->date)
		return -1;

	if (a->time < b->time)
		return 1;
	if (a->time > b->time)
		return -1;

	return 0;
}

static int rdr_logdir_rule_check(void)
{
	struct dir_array *dirs;
	int ret = 0;

	dirs = rdr_logdir_get(PATH_ROOT, true, true);
	if (IS_ERR_OR_NULL(dirs)) {
		BB_ERR("get dir info failed\n");
		return -1;
	}

	/*sort dirs*/
	sort(dirs->dir, dirs->dir_num, sizeof(struct dir_info),
	     rdr_logdir_sort_handle, NULL);

	ret = rdr_logdir_rule_process(dirs, rdr_get_logsize(),
				      rdr_get_lognum());
	rdr_logdir_show(dirs);
	rdr_logdir_put(dirs);

	return ret;
}

/*
 * Check the date string if it's valid.
 *
 * return value
 * return 0 for successful checking, otherwise failed.
 *
 */
static inline int rdr_check_date(const char *date, u32 len)
{
	u32 i;

	if (!date) {
		BB_ERR("parameter date invalid\n");
		return -1;
	}

	for (i = 0; i < len; i++)
		if (date[i] == '\0')
			break;
	if (i > 0)
		return 0;

	return -EINVAL;
}

static inline void rdr_logpath_init(u32 is_last)
{
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(is_last)];
	struct rdr_struct_s *tmpbb = NULL;
	char *last_data = NULL;
	int flag = atomic_read(&info->flag);

	/*The abnormal startup log directory can only be created once.*/
	if (is_last && (flag & LOGDIR_INIT)) {
		atomic_set(&info->flag, LOGDIR_USE | LOGDIR_INIT);
		return;
	}

	memset(info->date, 0, DATATIME_MAXLEN);
	if (is_last) {
		tmpbb = rdr_get_head(is_last);
		if (IS_ERR_OR_NULL(tmpbb))
			goto create_newdate;
		last_data = (char *)(tmpbb->base_info.datetime);
		if (unlikely(rdr_check_date(last_data, DATATIME_MAXLEN)))
			goto create_newdate;
		memcpy(info->date, last_data, sizeof(info->date));
		BB_PN("will use last exception datetime:[%s]\n", last_data);
		goto create_path;
	}

create_newdate:
	snprintf(info->date, DATATIME_MAXLEN, "%s-%08lld", rdr_get_timestamp(),
		 rdr_get_tick());
create_path:
	snprintf(info->path, PATH_MAXLEN, "%s%s/", PATH_ROOT, info->date);
	atomic_set(&info->flag, LOGDIR_USE | LOGDIR_INIT);
}

char *rdr_get_logdir_path(u32 is_last)
{
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(is_last)];
	int flag = atomic_read(&info->flag);

	if (!(flag & LOGDIR_USE)) {
		BB_ERR("rdr log[%d] saving is not start\n", is_last);
		return NULL;
	}

	return info->path;
}

char *rdr_get_logdir_date(u32 is_last)
{
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(is_last)];
	int flag = atomic_read(&info->flag);

	if (!(flag & LOGDIR_USE)) {
		BB_ERR("rdr log[%d] saving is not start\n", is_last);
		return NULL;
	}

	return info->date;
}

static inline int rdr_create_epath(u32 is_last)
{
	int ret;

	BB_PR_START();
	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return -1;

	ret = rdr_create_dir(rdr_get_logdir_path(is_last));
	BB_PR_END();

	return ret;
}

bool rdr_log_save_is_last(void)
{
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(false)];
	int flag = atomic_read(&info->flag);

	return !(flag & LOGDIR_USE);
}

int rdr_log_save_start(u32 is_last)
{
	int ret = 0;
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(is_last)];
	int flag = atomic_read(&info->flag);

	atomic_set(&info->flag, true);
	rdr_logpath_init(is_last);
	ret = rdr_create_epath(is_last);
	if (ret) {
		atomic_set(&info->flag, flag & (int)(~LOGDIR_USE));
		BB_ERR("create exception path error\n");
	}

	return ret;
}

void rdr_log_save_end(u32 is_last)
{
	struct date_path_info *info = &g_loginfo[LOGPATH_INDEX(is_last)];
	int flag = atomic_read(&info->flag);

	atomic_set(&info->flag, flag & (int)(~LOGDIR_USE));
	rdr_logdir_rule_check();
}

int rdr_dump_init(void)
{
	int ret;

	while (rdr_wait_partition(PATH_MNTN_PARTITION, RDR_TIME_OUT,
				  (S_IFDIR | S_IWUSR)) != 0)
		;

	ret = rdr_create_dir(PATH_ROOT);
	if (ret)
		return ret;

	/* according to authority requirements,hisi_logs and its subdir are root-system */
	ret = (int)rdr_chown((const char __user *)PATH_ROOT, ROOT_UID,
			     SYSTEM_GID, true);
	if (ret) {
		BB_ERR("chown %s uid [%d] gid [%d] failed err [%d]!\n",
		       PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
		return ret;
	}

	return 0;
}

void rdr_dump_exit(void)
{
}

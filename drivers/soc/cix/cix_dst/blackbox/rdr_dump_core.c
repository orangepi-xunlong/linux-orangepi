// SPDX-License-Identifier: GPL-2.0-only
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

#include <linux/sort.h>
#include <linux/cacheflush.h>
#include <linux/kmsg_dump.h>
#include "rdr_print.h"
#include "rdr_field.h"

#define DMESG_SAVE_MAXSIZE 0xc000
#define RAMLOG_SIZE 0x4000
#define RAM_LOG_PADDING_SIZE 100
#define RAMLOG_LEVEL(level) { #level, RAMLOG_##level }
#define RAMLOG_RRINT(format, ...) pr_info(format, ##__VA_ARGS__)
#define LOG_DEFAULT_SIZE 25
#define RAMLOG_LEVEL_START "\"level\":"
#define RAMLOG_TIME_START "\"time\":"
#define RAMLOG_LOG_START "\"log\":"

typedef struct {
	char level[10];
	u64 time;
	char *log;
} log_entry;

typedef struct {
	char *start;
	char *end;
	size_t len;
	log_entry log;
} str_range;

#undef RAMLOG_FLAG_DEF
#define RAMLOG_FLAG_DEF(name, val) MNTN_DEF_ARGS([val] = #name ".txt", )
static char *ramlog_name[] = { RAMLOG_LIST };

static int parse_log_entry(const char *input, log_entry *entry)
{
	char time[30] = { 0 };
	char *level_str, *time_str, *log_str;
	u64 level_offset, time_offset, log_offset;
	u64 level_len, time_len;

	memset(entry,0,sizeof(log_entry));

	level_str = strstr(input, RAMLOG_LEVEL_START);
	time_str = strstr(input, RAMLOG_TIME_START);
	log_str = strstr(input, RAMLOG_LOG_START);
	if (IS_ERR_OR_NULL(level_str) || IS_ERR_OR_NULL(time_str) ||
	    IS_ERR_OR_NULL(log_str)) {
		BB_PN("parse log entry failed, %s\n", input);
		return -1;
	}

	/*{"level":"INFO","time":"0x143","log":"Done power_domain:gpu_pd, off*/
	level_offset = level_str - input + 9;
	time_offset = time_str - input + 8;
	log_offset = log_str - input + 7;
	level_len = time_offset - 10 - level_offset;
	time_len = log_offset - 9 - time_offset;
	if (level_len > sizeof(entry->level) || time_len > sizeof(time)) {
		BB_PN("log len err, %s\n", input);
		return -1;
	}

	strncpy(entry->level, input + level_offset, level_len);
	entry->level[level_len] = '\0';
	strncpy(time, input + time_offset, time_len);
	time[time_len] = '\0';
	entry->log = (char *)input + log_offset;

	// str to long int
	if (kstrtou64(time, 16, &entry->time)) {
		BB_PN("log time err, %s\n", input);
		return -1;
	}

	return 0;
}

static void get_ramlog_timestamp(const char *buffer, size_t size,
				str_range *log_range)
{
	if (parse_log_entry(log_range->start, &log_range->log))
		log_range->log.time = -1;
}

static int ramlog_sort_handle(const void *va, const void *vb)
{
	const str_range *a = va;
	const str_range *b = vb;

	if (a->log.time < b->log.time)
		return -1;
	if (a->log.time > b->log.time)
		return 1;

	return 0;
}

/* parser all log in ringbuf */
static void parse_all_log_entries(struct file *fp, const char *buffer,
				  size_t size)
{
	char *current_ptr = NULL;
	char *start_of_log;
	char *end_of_log;
	str_range *log;
	u32 log_index = 0;
	log_entry *tmp_entry = NULL;
	bool err;
	char *process_log = NULL;

	start_of_log = strnstr(buffer, "{\"level", size);
	if (start_of_log == NULL)
		return;

	process_log = kzalloc(size + 100, GFP_KERNEL);
	if (IS_ERR_OR_NULL(process_log))
		return;
	memcpy(process_log, start_of_log, size - (start_of_log - buffer));
	memcpy(process_log + size - (start_of_log - buffer), buffer,
	       start_of_log - buffer);

	log = kcalloc(size / 2, sizeof(str_range), GFP_KERNEL);
	if (IS_ERR_OR_NULL(log)) {
		kfree(process_log);
		BB_ERR("Failed to allocate memory.\n");
		return;
	}

	current_ptr = process_log;
	do {
		start_of_log = strnstr(current_ptr, "{\"level",
				       size - (current_ptr - process_log));
		if (start_of_log == NULL) {
			/*No logs*/
			BB_DBG("%ld,not logs,%s\n",
			       size - (current_ptr - process_log), current_ptr);
			break;
		}

		end_of_log =
			strnstr(start_of_log, "};",
				(int)(size - (start_of_log - process_log)));
		if (end_of_log == NULL)
			break;

		if (log_index >= size / 2)
			break;
		log[log_index].start = start_of_log;
		log[log_index].end = end_of_log;
		*log[log_index].end = '\0';
		log[log_index].len = (size_t)(end_of_log - start_of_log);
		get_ramlog_timestamp(
			process_log, size, &log[log_index]);
		if (log[log_index].log.time != -1) {
			log_index++;
		}

		/* update current_ptr to next entry */
		current_ptr = end_of_log + 1;
		/* ringbuf */
		if (current_ptr >= process_log + size) {
			current_ptr = (char *)process_log;
		}
	} while (1);

	if (!log_index) {
		kfree(log);
		kfree(process_log);
		BB_DBG("not have log\n");
		return;
	}

	/*sort ramlog by timestamp*/
	sort(log, log_index, sizeof(str_range), ramlog_sort_handle, NULL);

	for (int i = 0; i < log_index; i++) {
		tmp_entry = &log[i].log;

		if (tmp_entry->level[0] != '\0')
			rdr_cleartext_print(fp, &err, "<%s>[%llu] %s",
					    tmp_entry->level, tmp_entry->time,
					    tmp_entry->log);
	}

	kfree(log);
	kfree(process_log);
}

void rdr_save_ramlog(const char *logpath, u32 flags, bool is_last)
{
	char *vaddr;
	u64 size;
	u32 bit;
	struct file *fp;
	char *log_addr, *start_addr;

	BB_PR_START();
	if (logpath == NULL) {
		BB_ERR("logpath is null");
		BB_PR_END();
		return;
	}

	vaddr = rdr_ramlog_mem().vaddr;
	size = rdr_ramlog_mem().size;

	dcache_inval_poc((unsigned long)vaddr,
			 (unsigned long)(vaddr + size - 1));

	log_addr = kzalloc(RAMLOG_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(log_addr)) {
		BB_ERR("no mem\n");
		return;
	}

	for (int i = 0; i < 32; i++) {
		bit = flags & BIT(i);
		if (!(bit & RDR_SAVE_RAMLOG))
			continue;
		memcpy(log_addr,
		       vaddr + RAMLOG_SIZE * (i - RAMLOG_FLAG_START_BIT),
		       RAMLOG_SIZE);

		if (is_last)
			start_addr = log_addr + (RAMLOG_SIZE >> 1);
		else
			start_addr = log_addr;

		fp = bbox_cleartext_get_filep(
			logpath, ramlog_name[i - RAMLOG_FLAG_START_BIT]);
		if (IS_ERR_OR_NULL(fp))
			continue;
		parse_all_log_entries(fp, start_addr,
				      (RAMLOG_SIZE >> 1) -
					      RAM_LOG_PADDING_SIZE);
		bbox_cleartext_end_filep(fp);
	}
	kfree(log_addr);

	BB_PR_END();
}

static void rdr_save_dmesg(const char *logpath)
{
	char *dmesg = NULL;
	struct kmsg_dump_iter iter;
	size_t size;

	if (logpath == NULL) {
		BB_ERR("logpath is null\n");
		return;
	}

	dmesg = kzalloc(DMESG_SAVE_MAXSIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(dmesg)) {
		BB_ERR("no mem\n");
		return;
	}
	kmsg_dump_rewind(&iter);
	if (!kmsg_dump_get_buffer(&iter, true, dmesg, DMESG_SAVE_MAXSIZE,
				  &size)) {
		BB_ERR("kmsg_dump_get_buffer fail\n");
		kfree(dmesg);
		return;
	}

	(void)rdr_savebuf2fs(logpath, "dmesg.txt", dmesg, size, 0);
	kfree(dmesg);
}

/*
 * func description:
 *  save exce info to history.log
 * return
 *  !0   fail
 *  == 0 success
 */
int rdr_save_history_log(struct rdr_exception_info_s *p, char *date,
			 u32 datelen, bool is_save_done, u32 bootup_keypoint)
{
	int ret = 0;

	char buf[HISTORY_LOG_SIZE];
	struct kstat historylog_stat;
	char local_path[PATH_MAXLEN];
	char *reboot_from_ap = NULL;
	char *subtype_name = NULL;

	if (p == NULL || date == NULL) {
		BB_ERR("invalid parameter, p_exce_info or date is null\n");
		return -1;
	}

	BB_PR_START();
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
	subtype_name = rdr_get_exception_subtype_name(p->e_exce_type,
						      p->e_exce_subtype);

	(void)snprintf(
		buf, HISTORY_LOG_SIZE,
		"system exception core [%s], reason [%s:%s], time [%s][%s], sysreboot [%s], bootup_keypoint [%u]\n",
		rdr_get_core_name_by_core(p->e_from_core),
		rdr_get_exception_type_name(p->e_exce_type), subtype_name, date,
		is_save_done ? "save_done" : "last_save_not_done",
		reboot_from_ap, bootup_keypoint);

	memset(local_path, 0, PATH_MAXLEN);
	(void)snprintf(local_path, PATH_MAXLEN, "%s/%s", PATH_ROOT,
		       "history.log");

	if (rdr_vfs_stat(local_path, &historylog_stat) == 0 &&
	    historylog_stat.size > HISTORY_LOG_MAX)
		rdr_rm_file(local_path); /* delete history.log */

	if (rdr_vfs_stat(PATH_ROOT, &historylog_stat) != 0) {
		ret = rdr_dump_init();
		if (ret) {
			BB_ERR("rdr_create_dir fail\n");
			return ret;
		}
	}

	(void)rdr_savebuf2fs(PATH_ROOT, "history.log", buf, strlen(buf), 1);

	BB_PR_END();
	return ret;
}

int rdr_save_history_log_for_undef_exception(struct rdr_syserr_param_s *p)
{
	char buf[HISTORY_LOG_SIZE];
	struct kstat historylog_stat;
	char local_path[PATH_MAXLEN];

	if (p == NULL) {
		BB_ERR("exception: NULL\n");
		return -1;
	}

	BB_PR_START();
	memset(buf, 0, HISTORY_LOG_SIZE);
	(void)snprintf(
		buf, HISTORY_LOG_SIZE,
		"system exception undef. modid[0x%x], arg [0x%x], arg [0x%x]\n",
		p->modid, p->arg1, p->arg2);

	memset(local_path, 0, PATH_MAXLEN);
	(void)snprintf(local_path, PATH_MAXLEN, "%s/%s", PATH_ROOT,
		       "history.log");

	if (rdr_vfs_stat(local_path, &historylog_stat) == 0 &&
	    historylog_stat.blksize > HISTORY_LOG_MAX)
		rdr_rm_file(local_path); /* delete history.log */

	(void)rdr_savebuf2fs(PATH_ROOT, "history.log", buf, strlen(buf), 1);

	BB_PR_END();
	return 0;
}

void rdr_save_log(const struct rdr_exception_info_s *p_exce_info)
{
	u32 save_flags;
	char *date;
	char *path;

	if (p_exce_info == NULL) {
		BB_ERR("invalid parameter. p_exce_info is null\n");
		return;
	}

	/* system(ap) reset, save logs in reboot */
	if (p_exce_info->e_reset_core_mask & RDR_AP) {
		BB_PN("system reset, no need to save\n");
		return;
	}

	path = rdr_get_logdir_path(false);
	if (IS_ERR_OR_NULL(path)) {
		BB_ERR("path is err\n");
		return;
	}

	rdr_save_baseinfo(path, false);

	save_flags = p_exce_info->e_save_log_flags;
	if (save_flags & RDR_SAVE_RAMLOG)
		rdr_save_ramlog(path, save_flags, false);

	if (save_flags & RDR_SAVE_DMESG)
		rdr_save_dmesg(path);

	date = rdr_get_logdir_date(false);
	if (!IS_ERR_OR_NULL(date))
		(void)rdr_save_history_log((void *)p_exce_info, date,
					   DATATIME_MAXLEN, true, 0);
}

void rdr_save_baseinfo(const char *logpath, bool is_last)
{
	void *addr = rdr_get_head(is_last);
	char *name = NULL;

	BB_PR_START();
	if (IS_ERR_OR_NULL(logpath) || IS_ERR_OR_NULL(addr)) {
		BB_ERR("logpath:%px, addr:%px\n", logpath, addr);
		BB_PR_END();
		return;
	}
	if (is_last)
		name = RDX_BIN;
	else
		name = RDR_BIN;
	/* save pbb to fs */
	(void)rdr_savebuf2fs_compressed(logpath, name, addr,
					rdr_total_mem_size(addr));

	BB_PR_END();
}

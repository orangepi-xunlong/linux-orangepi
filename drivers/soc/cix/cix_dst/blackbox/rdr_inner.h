/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * rdr_inner.h
 *
 * blackbox header file (blackbox: kernel run data recorder.)
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
#ifndef __BB_INNER_H__
#define __BB_INNER_H__

#include <linux/list.h>
#include <linux/of.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>

#define PATH_MAXLEN 128
#define RDR_LOG_PATH_LEN 32
#define TIME_MAXLEN 8
#define LOG_TIME_MAXLEN 10
#define DATA_MAXLEN 14
#define RDR_CMDWORD_MAXLEN 24
#define DATATIME_MAXLEN 24 /* 14+8 +2, 2: '-'+'\0' */
#define PATH_MEMDUMP "memorydump"
#define RDR_RAMLOG_BIN "ramlog.bin"
#define RDR_BIN "bbox.bin"
#define RDX_BIN "bbox_aft.bin"
#define BBOX_SPLIT_BIN "/bbox_split_bin/"
#define BBOX_HEAD_INFO "BBOX_HEAD_INFO"
#define ROOT_UID 0
#define SYSTEM_GID 1000
#define DIR_LIMIT (S_IRWXU | S_IRWXG)
#define FILE_LIMIT (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define RDR_DUMP_LOG_START 0x20120113
#define RDR_DUMP_LOG_DONE 0x20140607
#define RDR_PROC_EXEC_START 0xff115501
#define RDR_PROC_EXEC_DONE 0xff123059
#define RDR_REBOOT_DONE 0xff1230ff
#define RDR_CLEARTEXT_LOG_DONE 0x1
#define RDR_WAIT_PARTITION_TIME 1000

#define RDR_PRODUCT_VERSION "PRODUCT_VERSION_STR"
#define RDR_PRODUCT "PRODUCT_NAME"
#define RDR_PRODUCT_MAXLEN 16
#define BBOX_MODID_LAST_SAVE_NOT_DONE 0x8100fffe
#define RDR_MODID_AP_ABNORMAL_REBOOT 0x8100ffff
#define RDR_REBOOT_REASON_LEN 24
/* BBOX_COMMON_CALLBACK for public callback tags */
#define BBOX_COMMON_CALLBACK 0x1ull
#define BBOX_CALLBACK_MASK 0x3ull

#define WAIT_TIME 50
#define BBOX_RT_PRIORITY 98
#define HISTORY_LOG_SIZE 256
#define RDR_BUF_SIZE 1024
#define RDR_TIME_OUT 1000
#define HISTORY_LOG_MAX 0x400000 /* 64*16*1024*4 = 4M */

#define UNKNOWN_CORE_NAME "UNDEF"
#define RDR_CORE_INDEX_IS_ERR(index) \
	unlikely(index < RDR_CORE_MIN_INDEX || index >= RDR_CORE_MAX_INDEX)
#define RDR_CORE_IS_ERR(core)                                   \
	unlikely(core < RDR_CORE_MIN || core >= RDR_CORE_MAX || \
		 (core & (core - 1)) != 0)
#define RDR_CORE_2_CORE_INDEX(core) (ffs(core) - 1)
#define RDR_CORE_INDEX_2_CORE(index) (BIT(index))

struct exception_sub_word {
	uint type;
	char *word;
};

struct exception_word {
	uint type;
	char *word;
	struct exception_sub_word *sub;
	uint sub_num;
};

struct rdr_syserr_param_s {
	struct list_head syserr_list;
	u32 modid;
	u32 arg1;
	u32 arg2;
};

struct rdr_module_ops_s {
	struct list_head s_list;
	u64 s_core_id;
	struct rdr_module_ops_pub s_ops;
};

struct rdr_cleartext_ops_s {
	struct list_head s_list;
	u64 s_core_id;
	pfn_cleartext_ops ops_cleartext;
};

struct blackbox_modid_list {
	unsigned int modid_start;
	unsigned int modid_end;
	char *modid_str;
};

struct rdr_list_head {
	struct list_head *cur;
	struct list_head *next;
};

struct rdr_log_count {
	u32 size;
	u32 tmpsize;
	u32 rdr_max_logs;
	u32 rdr_log_nums;
};

struct rdr_area_data {
	u32 value;
	u32 data[RDR_CORE_MAX_INDEX];
};

extern char *rdr_core_name[];

/* blackbox internal function, not used in external modules */
void rdr_save_cleartext(bool is_last);
void rdr_execption_callback(struct rdr_exception_info_s *p_exce_info, u32 argc,
			    void *argv);
struct rdr_exception_info_s *rdr_get_exception_info(u32 modid);
void rdr_print_one_exc(struct rdr_exception_info_s *e);

u64 rdr_notify_module_dump(int dumpmod, u32 modid, struct rdr_exception_info_s *e_info,
			   char *path);
u64 rdr_notify_module_callback(u32 modid, struct rdr_exception_info_s *e_info);
bool rdr_module_is_register(u64 coreid);

int rdr_save_history_log(struct rdr_exception_info_s *p, char *date,
			 u32 datelen, bool is_save_done, u32 bootup_keypoint);
void rdr_save_log(const struct rdr_exception_info_s *p_exce_info);
void rdr_save_ramlog(const char *logpath, u32 flags, bool is_last);
int rdr_save_history_log_for_undef_exception(struct rdr_syserr_param_s *p);

void rdr_notify_module_reset(u32 modid, struct rdr_exception_info_s *e_info);

char *rdr_get_logdir_date(u32 is_last);
char *rdr_get_logdir_path(u32 is_last);
bool rdr_log_save_is_last(void);
int rdr_log_save_start(u32 is_last);
void rdr_log_save_end(u32 is_last);

int rdr_dump_init(void);
void rdr_dump_exit(void);
bool rdr_init_done(void);

/* kernel function. */
struct task_struct **get_last_task_ptr(void);

/*
 * func name: rdr_get_exception_type_name
 * get exception exce str for exce id.
 * func args:
 *    u64 e_exce_type
 * return value
 *     NULL  error
 *     !NULL exce str.
 */
char *rdr_get_exception_type_name(u32 e_exce_type);
uint32_t rdr_get_exception_type(char *name);
u32 rdr_get_reboot_type(void);
char *rdr_get_exception_subtype_name(u32 e_exce_type, u32 subtype);
u32 rdr_get_exec_subtype_value(void);

/*
 * func name: rdr_wait_partition
 * .
 * func args:
 *  char*  path,        path of watit file.
 *  u32 timeouts,       time out.
 * return
 *     <0 fail
 *     0  success
 */
bool rdr_syserr_list_empty(void);

int rdr_wait_partition(const char *path, int timeouts, int mode);
void rdr_get_builddatetime(u8 *out, u32 out_len);
u64 rdr_get_tick(void);
int rdr_get_suspend_state(void);
int rdr_get_reboot_state(void);
void rdr_set_saving_state(int state);

static inline char *rdr_get_core_name_by_index(u32 index)
{
	if (RDR_CORE_INDEX_IS_ERR(index))
		return UNKNOWN_CORE_NAME;

	return rdr_core_name[index];
}

static inline char *rdr_get_core_name_by_core(u64 core)
{
	if (RDR_CORE_IS_ERR(core))
		return UNKNOWN_CORE_NAME;

	return rdr_get_core_name_by_index(RDR_CORE_2_CORE_INDEX(core));
}

int rdr_common_early_init(struct platform_device *pdev);
int rdr_common_init(void);
bbox_mem rdr_ramlog_mem(void);
bbox_mem rdr_reserved_mem(void);
struct rdr_area_data rdr_get_area_data(void);
RDR_NVE rdr_get_nve(void);

void rdr_save_baseinfo(const char *logpath, bool is_last);
void rdr_cleartext_dumplog_done(void);
void rdr_field_dumplog_done(void);
void rdr_field_reboot_done(void);
void rdr_field_procexec_done(void);
void rdr_field_baseinfo_reinit(void);

char *blackbox_get_modid_str(u32 modid);
void bbox_save_done(const char *logpath, u32 step);
void rdr_flush_total_mem(void);
void rdr_record_reboot_times2mem(void);
void rdr_reset_reboot_times(void);
int rdr_record_reboot_times2file(void);
u32 rdr_get_reboot_times(void);
void rdr_cleartext_print_ops(void);
int rdr_exception_trace_record_ap(u64 e_reset_core_mask, u64 e_from_core,
				  u32 e_exce_type, u32 e_exce_subtype);
void incorrect_reboot_reason_analysis(char *dir_path,
				      struct rdr_exception_info_s *exception);
void record_exce_type(const struct rdr_exception_info_s *einfo);

static inline int rdr_saving_start(u32 is_last)
{
	bool ret;

	rdr_set_saving_state(1);
	ret = rdr_log_save_start(is_last);
	if (ret)
		rdr_set_saving_state(0);
	return ret;
}

static inline void rdr_saving_end(u32 is_last)
{
	rdr_set_saving_state(0);
	rdr_log_save_end(is_last);
}

#endif /* End #define __BB_INNER_H__ */

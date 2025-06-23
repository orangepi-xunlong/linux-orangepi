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

#define PATH_MAXLEN         128
#define RDR_LOG_PATH_LEN    32
#define TIME_MAXLEN         8
#define LOG_TIME_MAXLEN     10
#define DATA_MAXLEN         14
#define RDR_CMDWORD_MAXLEN  24
#define DATATIME_MAXLEN     24 /* 14+8 +2, 2: '-'+'\0' */
#define PATH_MEMDUMP        "memorydump"
#define RDR_RAMLOG_BIN      "ramlog.bin"
#define RDR_BIN             "bbox.bin"
#define RDX_BIN             "bbox_aft.bin"
#define DFX_BIN             "dfx.bin"
#define CONSOLE_RAMOOPS     "/proc/balong/pstore/console-ramoops"
#define PMSG_RAMOOPS        "/proc/balong/pstore/pmsg-ramoops-0"
#define BBOX_SPLIT_BIN      "/bbox_split_bin/"
#define BBOX_HEAD_INFO      "BBOX_HEAD_INFO"
#define ROOT_UID            0
#define SYSTEM_GID          1000
#define DIR_LIMIT           (S_IRWXU | S_IRWXG)
#define FILE_LIMIT          (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define RDR_DUMP_LOG_START  0x20120113
#define RDR_DUMP_LOG_DONE   0x20140607
#define RDR_PROC_EXEC_START 0xff115501
#define RDR_PROC_EXEC_DONE  0xff123059
#define RDR_REBOOT_DONE     0xff1230ff
#define RDR_CLEARTEXT_LOG_DONE 0x1
#define RDR_PNAME_SIZE      (BDEVNAME_SIZE + 12)
#define RDR_WAIT_PARTITION_TIME 1000

#define SRC_DUMPEND  "/proc/balong/memory/dump_end"

#define RDR_PRODUCT_VERSION "PRODUCT_VERSION_STR"
#define RDR_PRODUCT "PRODUCT_NAME"
#define RDR_PRODUCT_MAXLEN              16
#define BBOX_MODID_LAST_SAVE_NOT_DONE   0x8100fffe
#define RDR_MODID_AP_ABNORMAL_REBOOT    0x8100ffff
#define RDR_REBOOT_REASON_LEN           24
/* BBOX_COMMON_CALLBACK for public callback tags */
#define BBOX_COMMON_CALLBACK            0x1ull
#define BBOX_CALLBACK_MASK              0x3ull

#define WAIT_TIME           50
#define BBOX_RT_PRIORITY    98
#define HISTORY_LOG_SIZE    256
#define RDR_BUF_SIZE        1024
#define RDR_COUNT_MAX       1000
#define RDR_TIME_OUT        1000
#define HISTORY_LOG_MAX     0x400000 /* 64*16*1024*4 = 4M */

struct cmdword {
	unsigned char name[RDR_CMDWORD_MAXLEN];
	unsigned int num;
	unsigned char category_name[RDR_CMDWORD_MAXLEN];
	unsigned int category_num;
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
	unsigned int modid_span_little;
	unsigned int modid_span_big;
	char *modid_str;
};

struct rdr_file_attr {
	unsigned int uid;
	unsigned int gid;
};

struct rdr_buf_attr {
	int bpos;
	char buf[RDR_BUF_SIZE];
	int bufsize;
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
	u32 data[RDR_CORE_MAX];
};

struct bbox_area_core_data {
	u32 size;
	char *addr;
	char *bbox_area_names;
};

typedef struct bbox_mem {
	void *vaddr;
	u64 paddr;
	u64 size;
} bbox_mem;

/* blackbox internal function, not used in external modules */
u32 get_cleartext_test(void);
/* blackbox internal function, not used in external modules */
struct semaphore *get_cleartext_sem(void);

void rdr_callback(struct rdr_exception_info_s *p_exce_info, u32 mod_id,
				char *logpath, u32 logpath_len);
struct rdr_exception_info_s *rdr_get_exception_info(u32 modid);
void rdr_print_one_exc(struct rdr_exception_info_s *e);

u64 rdr_notify_module_dump(u32 modid, struct rdr_exception_info_s *e_info, char *path);
u64 rdr_notify_onemodule_dump(u32 modid, u64 core, u32 type, u64 formcore, char *path);
u64 rdr_get_cur_regcore(void);

u64 rdr_get_dump_result(u32 modid);
int rdr_save_history_log(struct rdr_exception_info_s *p, char *date, u32 datelen,
				bool is_save_done, u32 bootup_keypoint);
void rdr_save_pstore_log(const struct rdr_exception_info_s *p_exce_info, const char *path);
int rdr_save_history_log_for_undef_exception(struct rdr_syserr_param_s *p);

void rdr_notify_module_reset(u32 modid, struct rdr_exception_info_s *e_info);

int rdr_create_exception_path(struct rdr_exception_info_s *e, char *path, char *date,
				u32 datelen);
int bbox_create_dfxlog_path(char *path, char *date, u32 data_len);
int rdr_create_epath_bc(char *path, u32 path_len);

void rdr_count_size(void);
bool rdr_check_log_rights(void);

int rdr_dump_init(void);
void rdr_dump_exit(void);
bool rdr_init_done(void);
int rdr_demo_init(void);

typedef void (*parse_record_t) (u64 *data, u32 len);

/* kernel function. */
struct task_struct **get_last_task_ptr(void);
unsigned int skp_skp_flag(void);
u64 skp_skp_resizeaddr(void);
void hisi_dump_bootkmsg(void);
#ifdef CONFIG_GCOV_KERNEL
void gcov_get_gcda(void);
#endif

/*
 * func name: rdr_get_exception_core
 * get exception core str for core id.
 * func args:
 *    u64 coreid
 * return value
 *    NULL  error
 *    !NULL core str.
 */
char *rdr_get_exception_core(u64 coreid);

/*
 * func name: rdr_get_exception_core
 * get exception core str for core id.
 * func args:
 *    u64 coreid
 * return value
 *     NULL  error
 *     !NULL core str.
 */
char *rdr_get_exception_type(u64 coreid);

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

int rdr_wait_partition(const char *path, int timeouts);
void rdr_get_builddatetime(u8 *out, u32 out_len);
u64 rdr_get_tick(void);
int rdr_get_suspend_state(void);
int rdr_get_reboot_state(void);
void rdr_set_saving_state(int state);

int rdr_common_early_init(struct platform_device *pdev);
int rdr_common_init(void);
bbox_mem rdr_ramlog_mem(void);
bbox_mem rdr_reserved_mem(void);
struct rdr_area_data rdr_get_area_data(void);
int rdr_get_dumplog_timeout(void);
RDR_NVE rdr_get_nve(void);

void rdr_save_last_baseinfo(const char *logpath);
void rdr_save_cur_baseinfo(const char *logpath);
void rdr_save_ramlog(const char *logpath);

char *rdr_field_get_datetime(void);
void rdr_cleartext_dumplog_done(void);
void rdr_field_dumplog_done(void);
void rdr_field_reboot_done(void);
void rdr_field_procexec_done(void);
void rdr_field_baseinfo_reinit(void);

int rdr_get_sr_state(void);
int rdr_get_reboot_state(void);

int rdr_bootcheck_thread_body(void *arg);
char *rdr_get_reboot_logpath(void);
struct cmdword *get_reboot_reason_map(void);
u32 get_reboot_reason_map_size(void);
char *rdr_get_reboot_reason(void);
u32 rdr_get_reboot_type(void);

char *blackbox_get_modid_str(u32 modid);
int bbox_chown(const char *path, uid_t user, gid_t group, bool recursion);
void bbox_save_done(const char *logpath, u32 step);
void rdr_record_reboot_times2mem(void);
void rdr_reset_reboot_times(void);
void rdr_record_erecovery_reason(void);
int rdr_record_reboot_times2file(void);
u32 rdr_get_reboot_times(void);
void save_dfxpartition_to_file(void);
bool is_need_save_dfx2file(void);
void get_dfx_core_info(u32 *dfx_size_tbl, u32 size_tbl_len, u32 *dfx_addr_tbl, u32 addr_tbl_len);
bool need_save_mntndump_log(u32 reboot_type_s);
int rdr_create_dir(const char *path);
void bbox_cleartext_proc(const char *path, const char *base_addr);
void rdr_exceptionboot_save_cleartext(void);
int rdr_cleartext_body(void *arg);
int bbox_get_every_core_area_info(u32 *value, u32 *data, u32 datalen);
void rdr_cleartext_print_ops(void);
void rdr_hisiap_dump_root_head(u32 modid, u32 etype, u64 coreid);
void save_hisiap_log(char *log_path, u32 modid);
int rdr_exception_trace_init(void);
int rdr_exception_trace_record(u64 e_reset_core_mask, u64 e_from_core,
				u32 e_exce_type, u32 e_exce_subtype);
int rdr_exception_trace_cleartext_print(const char *dir_path, u64 log_addr, u32 log_len);
int get_every_core_exception_info(u32 *num, u32 *size, u32 sizelen);
int ap_awdt_analysis(struct rdr_exception_info_s *exception);
void incorrect_reboot_reason_analysis(char *dir_path, struct rdr_exception_info_s *exception);
int exception_trace_buffer_init(u8 *addr, unsigned int size);
int rdr_exception_analysis_ap(u64 etime, u8 *addr, u32 len,
				struct rdr_exception_info_s *exception);
int get_pmu_reset_base_addr(void);
void record_exce_type(const struct rdr_exception_info_s *einfo);
void read_dump_end(void);
extern unsigned int get_boot_into_recovery_flag(void);
char *rdr_get_subtype_name(u32 e_exce_type, u32 subtype);
char *rdr_get_category_name(u32 e_exce_type, u32 subtype);
u32 rdr_get_exec_subtype_value(void);
#endif /* End #define __BB_INNER_H__ */

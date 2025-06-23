/*
 * rdr_pub.h
 *
 * blackbox header file (blackbox: kernel run data recorder.).
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef __BB_PUB_H__
#define __BB_PUB_H__

#include <linux/module.h>
#include <linux/soc/cix/rdr_types.h>
#include <mntn_public_interface.h>
#include <linux/soc/cix/util.h>

#define STR_MODULENAME_MAXLEN 16
#define STR_EXCEPTIONDESC_MAXLEN 48
#define STR_TASKNAME_MAXLEN 16
#define STR_USERDATA_MAXLEN 64

#ifndef CONFIG_CIX_DFD_PHONE
#define PATH_MNTN_PARTITION "/var/log"
#define PATH_ROOT "/var/log/cix/bbox"
#define RDR_REBOOT_TIMES_FILE "/var/log/cix/bbox/reboot_times.log"
#else
#define PATH_MNTN_PARTITION "/data/lost+found"
#define PATH_ROOT "/data/hisi_logs/"
#define RDR_REBOOT_TIMES_FILE "/data/hisi_logs/reboot_times.log"
#endif

#define RDR_ERECOVERY_REASON_FILE "/cache/recovery/last_erecovery_entry"
#define RDR_UNEXPECTED_REBOOT_MARK_ADDR 0x2846579

#define INT_IN_FLAG 0xAAAAUL
#define INT_EXIT_FLAG 0xBBBBUL

/* Indicates the name of the flag file that stores the abnormal file directory log */
#define BBOX_SAVE_DONE_FILENAME "/DONE"

/* The macro definition of version detection starts */
#define FILE_EDITION "/proc/log-usertype"
#define OVERSEA_USER 5
#define BETA_USER 3
#define COMMERCIAL_USER 1

#define START_CHAR_0 '0'
#define END_CHAR_9 '9'

/* The macro definition of version detection is complete */
enum EDITION_KIND {
	EDITION_USER = 1,
	EDITION_INTERNAL_BETA = 2,
	EDITION_OVERSEA_BETA = 3,
	EDITION_MAX
};

/* Flag indicating that the log is saved after an exception occurs */
enum SAVE_STEP {
	BBOX_SAVE_STEP1 = 0x1,
	BBOX_SAVE_STEP2 = 0x2,
	BBOX_SAVE_STEP3 = 0x3,
	BBOX_SAVE_STEP_DONE = 0x100
};

/* this is for test */
enum rdr_except_reason_e {
	RDR_EXCE_WD = 0x01, /* watchdog timeout */
	RDR_EXCE_INITIATIVE, /* initictive call sys_error */
	RDR_EXCE_PANIC, /* ARM except(eg:data abort) */
	RDR_EXCE_STACKOVERFLOW,
	RDR_EXCE_DIE,
	RDR_EXCE_UNDEF,
	RDR_EXCE_MAX
};

enum PROCESS_PRI {
	RDR_ERR = 0x01,
	RDR_WARN,
	RDR_OTHER,
	RDR_PPRI_MAX
};

enum REBOOT_PRI {
	RDR_REBOOT_NOW = 0x01,
	RDR_REBOOT_WAIT,
	RDR_REBOOT_NO,
	RDR_REBOOT_MAX
};

enum REENTRANT {
	RDR_REENTRANT_ALLOW = 0xff00da00,
	RDR_REENTRANT_DISALLOW
};

enum UPLOAD_FLAG {
	RDR_UPLOAD_YES = 0xff00fa00,
	RDR_UPLOAD_NO
};

enum RDR_RETURN {
	RDR_SUCCESSED = 0x9f000000,
	RDR_FAILD = 0x9f000001,
	RDR_NULLPOINTER = 0x9f0000ff
};

enum RDR_SAVE_LOG_FLAG {
	RDR_SAVE_DMESG = (0x1 << 0),
	RDR_SAVE_CONSOLE_MSG = (0x1 << 1),
	RDR_SAVE_BL31_LOG = (0x1 << 2), /* the bl31 offset is 2 */
	RDR_SAVE_LOGBUF = (0x1 << 3), /* the logbuf offset is 3 */
};

typedef void (*rdr_e_callback)(u32, void*);

/*
 *   struct list_head   e_list;
 *   u32 modid, exception id;
 *   if modid equal 0, will auto generation modid, and return it.
 *   u32 modid_end, can register modid region. [modid~modid_end];
 *   need modid_end >= modid,
 *   if modid_end equal 0, will be register modid only,
 *   but modid & modid_end cant equal 0 at the same time.
 *   u32 process_priority, exception process priority
 *   u32 reboot_priority, exception reboot priority
 *   u64 save_log_mask, need save log mask
 *   u64 notify_core_mask, need notify other core mask
 *   u64 reset_core_mask, need reset other core mask
 *   u64 from_core, the core of happen exception
 *   u32 reentrant, whether to allow exception reentrant
 *   u32 exce_type, the type of exception
 *   char* from_module, the module of happen excption
 *   char* desc, the desc of happen excption
 *   rdr_e_callback callback, will be called when excption has processed.
 *   u32 save_log_flags, set bit 1 to save the log(dmsg, console, bl31log)
 *   u32 reserve_u32; reserve u32
 *   void* reserve_p reserve void *
 */
struct rdr_exception_info_s {
	struct list_head e_list;
	u32	e_modid;
	u32	e_modid_end;
	u32	e_process_priority;
	u32	e_reboot_priority;
	u64	e_notify_core_mask;
	u64	e_reset_core_mask;
	u64	e_from_core;
	u32	e_reentrant;
	u32	e_exce_type;
	u32	e_exce_subtype;
	u32	e_upload_flag;
	u8	e_from_module[MODULE_NAME_LEN];
	u8	e_desc[STR_EXCEPTIONDESC_MAXLEN];
	u32	e_save_log_flags;
	u32	e_reserve_u32;
	void *e_reserve_p;
	rdr_e_callback e_callback;
};

/*
 * func args:
 * u32 modid
 * exception id
 * u64 coreid
 * which core done
 * return value null
 */
typedef void (*pfn_cb_dump_done)(u32 modid, u64 coreid);

/*
 * func args:
 * u32 modid
 * exception id
 * u64 coreid
 * exception core
 * u32 etype
 * exception type
 * char *logpath
 * exception log path
 * pfn_cb_dump_done fndone
 * return mask bitmap.
 */
typedef void (*pfn_dump)(u32 modid, u32 etype, u64 coreid,
				char *logpath, pfn_cb_dump_done fndone);
/*
 * func args:
 * u32 modid
 * exception id
 * u32 coreid
 * exception core
 * u32 e_type
 * exception type
 * return value null
 */
typedef void (*pfn_reset)(u32 modid, u32 etype, u64 coreid);

/*
 * func args:
 * log_dir_path: the direcotory path of the file to be written in clear text format
 * u64 log_addr: the start address of the reserved memory for each core
 * u32 log_len: the length of the reserved memory for each core
 *
 * Attention:
 * the user can't dump through it's saved dump address but must in use of the log_addr
 *
 * return value
 * < 0 error
 * >=0 success
 */
typedef int (*pfn_cleartext_ops)(const char *log_dir_path, u64 log_addr, u32 log_len);


typedef int (*pfn_exception_init_ops)(u8 *phy_addr, u8 *virt_addr, u32 log_len);

typedef int (*pfn_exception_analysis_ops)(u64 etime, u8 *addr, u32 len,
				struct rdr_exception_info_s *exception);

struct rdr_module_ops_pub {
	pfn_dump    ops_dump;
	pfn_reset   ops_reset;
};

struct rdr_register_module_result {
	u64   log_addr;
	u32     log_len;
	RDR_NVE nve;
};

#ifdef CONFIG_PLAT_BBOX
/*
 * func args:
 * struct rdr_exception_info_pub* s_e_type
 * return value e_modid
 * < 0 error
 * >=0 success
 */
u32 rdr_register_exception(struct rdr_exception_info_s *e);

/*
 * func args:
 *   u32 modid, exception id;
 * return
 * < 0 fail
 * >=0 success
 */
int rdr_unregister_exception(u32 modid);

/*
 * func args:
 * @paddr: physical address in black box
 * @size: size of memory
 * return:
 * success: virtual address
 * fail: NULL or -ENOMEM
 */
void *rdr_bbox_map(phys_addr_t paddr, size_t size);

/*
 * func args:
 * @addr: virtual address that alloced by hisi_bbox_map
 */
void rdr_bbox_unmap(const void *vaddr);

/*
 * func args:
 * u32 coreid, core id;
 * struct rdr_module_ops_pub* ops;
 * struct rdr_register_module_result* retinfo
 *
 * return value e_modid
 * < 0 error
 * >=0 success
 */
int rdr_register_module_ops(
	u64 coreid,
	struct rdr_module_ops_pub *ops,
	struct rdr_register_module_result *retinfo);

/*
 * func args:
 * u64 coreid, core id;
 * return
 * < 0 fail
 * >=0 success
 */
int rdr_unregister_module_ops(u64 coreid);

/*
 * func args:
 * u32 modid, modid( must be registered);
 * u32 arg1, arg1;
 * u32 arg2, arg2;
 * char *data, short message.
 * u32 length, len(IMPORTANT: <=4k)
 * return void
 */
void rdr_system_error(u32 modid, u32 arg1, u32 arg2);

void rdr_syserr_process_for_ap(u32 modid, u64 arg1, u64 arg2);

/*
 * append(save) data to path.
 * func args:
 * struct file *fp: the pointer of file which to save the clear text.
 * bool *error: to fast the cpu process when there is error happened before nowadays print, please
 * refer the function bbox_head_cleartext_print to get the use of this parameter.
 *
 * return
 */
void rdr_cleartext_print(struct file *fp, bool *error, const char *format, ...);

/*
 * func args:
 * u64 core_id: the same with the parameter coreid of function rdr_register_module_ops
 * pfn_cleartext_ops ops_fn: the function to write the content of reserved memory in clear text format
 *
 * return value
 * < 0 error
 * 0 success
 */
int rdr_register_cleartext_ops(u64 coreid, pfn_cleartext_ops ops_fn);

/*
 *
 * Get the file descriptor pointer whose abosolute path composed by the dir_path&file_name
 * and initialize it.
 *
 * func args:
 * char *dir_path: the directory path about the specified file.
 * char *file_name:the name of the specified file.
 *
 * return
 * file descriptor pointer when success, otherwise NULL.
 *
 * attention
 * the function bbox_cleartext_get_filep shall be used
 * in paired with function bbox_cleartext_end_filep.
 */
struct file *bbox_cleartext_get_filep(const char *dir_path, char *file_name);

/*
 * cleaning of the specified file
 *
 * func args:
 * struct file *fp: the file descriptor pointer .
 * char *dir_path: the directory path about the specified file.
 * char *file_name:the name of the specified file.
 *
 * return
 *
 * attention
 * the function bbox_cleartext_end_filep shall be used
 * in paired with function bbox_cleartext_get_filep.
 */
void bbox_cleartext_end_filep(struct file *fp, const char *dir_path, char *file_name);

/*
 * func args:
 * void
 * return:
 * unsigned int: Version information is returned
 * 0x01 USER
 * 0x02 INTERNAL BETA
 * 0x03 OVERSEA BETA
 *
 * This function accesses the data partition of the user and therefore
 * depends on the correct mounting of the file system.
 * There is no timeout mechanism. The process will be accessed when the file system is mounted.
 * A sleep that is uncertain. In the preceding scenario, this interface cannot
 * be invoked in scenarios where sleep is not allowed.
 */
unsigned int rdr_check_edition(void);
int rdr_wait_partition(const char *path, int timeouts);
void rdr_set_wdt_kick_slice(u64 kickslice);
u64 rdr_get_last_wdt_kick_slice(void);
u64 get_32k_abs_timer_value(void);
void save_log_to_dfx_tempbuffer(u32 reboot_type);
void clear_dfx_tempbuffer(void);
void systemerror_save_log2dfx(u32 reboot_type);
u64 rdr_get_logsize(void);
u32 rdr_get_diaginfo_size(void);
u32 rdr_get_lognum(void);
char *rdr_get_timestamp(void);
void *bbox_vmap(phys_addr_t paddr, size_t size);
struct rdr_exception_info_s *rdr_get_exce_info(void);
#else
static inline void *rdr_bbox_map(phys_addr_t paddr, size_t size) { return NULL; }
static inline u32 rdr_register_exception(struct rdr_exception_info_s *e) { return 0; }
static inline int rdr_unregister_exception(u32 modid) { return 0; }
static inline int rdr_register_module_ops(
	u64 coreid,
	struct rdr_module_ops_pub *ops,
	struct rdr_register_module_result *retinfo) { return -1; }
static inline int rdr_unregister_module_ops(u64 coreid) { return 0; }
static inline void rdr_system_error(u32 modid, u32 arg1, u32 arg2) {}
static inline void rdr_syserr_process_for_ap(u32 modid, u64 arg1, u64 arg2) {}
static inline unsigned int bbox_check_edition(void) {return EDITION_USER; }
static inline int rdr_wait_partition(const char *path, int timeouts) { return 0; }
static inline void rdr_set_wdt_kick_slice(u64 kickslice) { return; }
static inline u64 rdr_get_last_wdt_kick_slice(void) { return 0; }
static inline u64 get_32k_abs_timer_value(void) { return 0; }
static inline void save_log_to_dfx_tempbuffer(u32 reboot_type) {return; };
static inline void clear_dfx_tempbuffer(void) {return; };
static inline void systemerror_save_log2dfx(u32 reboot_type) {return; }
static inline void rdr_bbox_unmap(const void *vaddr) {return; }
static inline u64 rdr_get_logsize(void) {return 0; }
static inline u32 rdr_get_diaginfo_size(void) {return 0; }
static inline u32 rdr_get_lognum(void) {return 0; }
static inline char *rdr_get_timestamp(void) {return NULL; }
static inline void *bbox_vmap(phys_addr_t paddr, size_t size) {return NULL; }
#endif

void get_exception_info(unsigned long *buf, unsigned long *buf_len);
#define RDR_REBOOTDUMPINFO_FLAG     0xdd140607

#endif/* End #define __BB_PUB_H__ */


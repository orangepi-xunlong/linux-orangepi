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
#define PATH_MNTN_PARTITION "/var/log/"
#define PATH_ROOT "/var/log/cix/bbox/"
#define RDR_REBOOT_TIMES_FILE "/var/log/cix/bbox/reboot_times.log"
#else
#define PATH_MNTN_PARTITION "/data/lost+found"
#define PATH_ROOT "/data/cix/"
#define RDR_REBOOT_TIMES_FILE "/data/cix/reboot_times.log"
#endif

#define RDR_UNEXPECTED_REBOOT_MARK_ADDR 0x2846579

/* Indicates the name of the flag file that stores the abnormal file directory log */
#define BBOX_SAVE_DONE_FILENAME "/DONE"

#define DEF_EXCE_STRUCT_RANGE(modid, modid_end, proc_prio, reboot_type,      \
			      dump_cores, reset_cores, from_core, exec_type, \
			      subtype, desc, log_flag, callback)             \
	{ .e_list = { 0, 0 },                                                \
	  .e_modid = (modid),                                                \
	  .e_modid_end = (modid_end),                                        \
	  .e_process_priority = (proc_prio),                                 \
	  .e_reboot_priority = (reboot_type),                                \
	  .e_notify_core_mask = (dump_cores),                                \
	  .e_reset_core_mask = (reset_cores),                                \
	  .e_from_core = (from_core),                                        \
	  .e_reentrant = RDR_REENTRANT_DISALLOW,                             \
	  .e_exce_type = (exec_type),                                        \
	  .e_exce_subtype = (subtype),                                       \
	  .e_upload_flag = RDR_UPLOAD_NO,                                    \
	  .e_from_module = #exec_type,                                       \
	  .e_desc = (desc),                                                  \
	  .e_save_log_flags = (log_flag),                                    \
	  .e_reserve_u32 = 0,                                                \
	  .e_reserve_p = 0,                                                  \
	  .e_callback = (callback) }

#define DEF_EXCE_STRUCT_SINGLE(proc_prio, reboot_type, dump_cores,         \
			       reset_cores, from_core, exec_type, subtype, \
			       desc, log_flag, callback)                   \
	{ .e_list = { 0, 0 },                                              \
	  .e_modid = MODID_##subtype,                                      \
	  .e_modid_end = MODID_##subtype,                                  \
	  .e_process_priority = (proc_prio),                               \
	  .e_reboot_priority = (reboot_type),                              \
	  .e_notify_core_mask = (dump_cores),                              \
	  .e_reset_core_mask = (reset_cores),                              \
	  .e_from_core = (from_core),                                      \
	  .e_reentrant = RDR_REENTRANT_DISALLOW,                           \
	  .e_exce_type = (exec_type),                                      \
	  .e_exce_subtype = (subtype),                                     \
	  .e_upload_flag = RDR_UPLOAD_NO,                                  \
	  .e_from_module = #exec_type,                                     \
	  .e_desc = (desc),                                                \
	  .e_save_log_flags = (log_flag),                                  \
	  .e_reserve_u32 = 0,                                              \
	  .e_reserve_p = 0,                                                \
	  .e_callback = (callback) }

#define RDR_BCH_M CONFIG_PLAT_BBOX_BCH_M
#define RDR_BCH_T CONFIG_PLAT_BBOX_BCH_T
#define RDR_BCH_MAX_INT                                             \
	(DIV_ROUND_UP(BIT(RDR_BCH_M) - (1 + RDR_BCH_M * RDR_BCH_T), \
		      8 * sizeof(uint)) -                           \
	 1)
#define RDR_BCH_MAX_BYTES (RDR_BCH_MAX_INT * sizeof(int))
#define RDR_BCH_ECC_BYTES (DIV_ROUND_UP(RDR_BCH_M * RDR_BCH_T, 8))
#define RDR_BCH_GET_ECC_BYTES(size) \
	(DIV_ROUND_UP((size), RDR_BCH_MAX_BYTES) * RDR_BCH_ECC_BYTES)

/* Flag indicating that the log is saved after an exception occurs */
enum SAVE_STEP {
	BBOX_SAVE_STEP1 = 0x1,
	BBOX_SAVE_STEP2 = 0x2,
	BBOX_SAVE_STEP3 = 0x3,
	BBOX_SAVE_STEP_DONE = 0x100
};

enum PROCESS_PRI { RDR_ERR = 0x01, RDR_WARN, RDR_OTHER, RDR_PPRI_MAX };

enum REBOOT_PRI {
	RDR_REBOOT_NOW = 0x01,
	RDR_REBOOT_WAIT,
	RDR_REBOOT_NO,
	RDR_REBOOT_MAX
};

enum REENTRANT { RDR_REENTRANT_ALLOW = 0xff00da00, RDR_REENTRANT_DISALLOW };

enum UPLOAD_FLAG { RDR_UPLOAD_YES = 0xff00fa00, RDR_UPLOAD_NO };

#undef RAMLOG_FLAG_DEF
#define RAMLOG_FLAG_START_BIT 4
#define RAMLOG_FLAG_DEF(name, val) MNTN_DEF_ARGS(RAMLOG_##name = (val), )
#define RAMLOG_LIST                                                            \
	MNTN_DEF_ARGS(RAMLOG_FLAG_DEF(CSUSE, 0) RAMLOG_FLAG_DEF(               \
		SF, 1) RAMLOG_FLAG_DEF(HIFI, 2) RAMLOG_FLAG_DEF(BL31, 3)       \
			      RAMLOG_FLAG_DEF(UEFI, 4) RAMLOG_FLAG_DEF(TEE, 5) \
				      RAMLOG_FLAG_DEF(PM, 6)                   \
					      RAMLOG_FLAG_DEF(BL2, 7))

enum ramlog_core { RAMLOG_LIST };

#undef RAMLOG_FLAG_DEF
#define RAMLOG_FLAG_DEF(name, val) \
	MNTN_DEF_ARGS(RDR_SAVE_##name = BIT(RAMLOG_FLAG_START_BIT + (val)), )
enum RDR_SAVE_LOG_FLAG {
	RDR_SAVE_DMESG = (0x1 << 0),
	RAMLOG_LIST
#undef RAMLOG_FLAG_DEF
#define RAMLOG_FLAG_DEF(name, val) MNTN_DEF_ARGS(RDR_SAVE_##name |)
		RDR_SAVE_RAMLOG = RAMLOG_LIST 0
};

typedef void (*rdr_e_callback)(u32, void *);

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
	u32 e_modid;
	u32 e_modid_end;
	u32 e_process_priority;
	u32 e_reboot_priority;
	u64 e_notify_core_mask;
	u64 e_reset_core_mask;
	u64 e_from_core;
	u32 e_reentrant;
	u32 e_exce_type;
	u32 e_exce_subtype;
	u32 e_upload_flag;
	u8 e_from_module[MODULE_NAME_LEN];
	u8 e_desc[STR_EXCEPTIONDESC_MAXLEN];
	u32 e_save_log_flags;
	u32 e_reserve_u32;
	void *e_reserve_p;
	rdr_e_callback e_callback;
};

typedef struct bbox_mem {
	void *vaddr;
	u64 paddr;
	u64 size;
} bbox_mem;

struct rdr_safemem {
	union {
		struct {
			uint id;
			uint res;
			u64 paddr;
			void *vaddr;
			u64 size;
		};
		uint rawdata[RDR_BCH_MAX_INT];
	} safe;
	uint8_t ecc[RDR_BCH_ECC_BYTES];
};

struct rdr_safemem_pool {
	uint magic;
	char name[MODULE_NAME_LEN];
	uint maxnum;
	uint curnum;
	bool low_to_high;
	void *base_alloc_addr;
	u64 phyaddr;
	void *end_alloc_addr;
	void *cur_alloc_addr;
	u64 pool_size;
	spinlock_t lock;
	struct rdr_safemem mem[];
};

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
 */
typedef void (*pfn_dump)(u32 modid, u32 etype, u64 coreid, char *logpath);
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
typedef int (*pfn_cleartext_ops)(const char *log_dir_path, u64 log_addr,
				 u32 log_len);

struct rdr_module_ops_pub {
	pfn_dump ops_dump;
	pfn_dump ops_dump_no_irq;
	pfn_reset ops_reset;
	void (*ops_callback)(u32 modid, u32 etype, u64 coreid);
};

struct rdr_register_module_result {
	u64 log_addr;
	u32 log_len;
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
int rdr_register_module_ops(u64 coreid, struct rdr_module_ops_pub *ops,
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
 *
 * return
 *
 * attention
 * the function bbox_cleartext_end_filep shall be used
 * in paired with function bbox_cleartext_get_filep.
 */
void bbox_cleartext_end_filep(struct file *fp);
void rdr_sys_sync(void);
u64 get_32k_abs_timer_value(void);
u64 rdr_get_logsize(void);
u32 rdr_get_lognum(void);
char *rdr_get_timestamp(void);
struct rdr_exception_info_s *rdr_get_exce_info(void);
int rdr_bch_encode(char *data, uint data_len, char *ecc, uint ecc_len);
int rdr_bch_checkout(char *data, uint data_len, char *ecc, uint ecc_len);
/*
 * mem_pool init
 *
 * func args:
 * pool: mem pool pointer.
 * name: mem pool name.
 * size: rdr_safemem_pool size.
 * pool_size: can alloc mem max size.
 * base_addr: alloc base address.
 * low_to_high: alloc mem from low to high.
 *
 * return: 0 isok.
 */
int rdr_safemem_pool_init(struct rdr_safemem_pool *pool, char *name, uint size,
			  bbox_mem *mem, bool low_to_high);
int rdr_safemem_pool_reinit(struct rdr_safemem_pool *pool);
int rdr_safemem_pool_show(struct rdr_safemem_pool *pool);
int rdr_safemem_alloc(struct rdr_safemem_pool *pool, uint id, uint size,
		      bbox_mem *mem);
int rdr_safemem_get(struct rdr_safemem_pool *pool, uint id, bbox_mem *mem);

#else
static inline void *rdr_bbox_map(phys_addr_t paddr, size_t size)
{
	return NULL;
}
static inline u32 rdr_register_exception(struct rdr_exception_info_s *e)
{
	return 0;
}
static inline int rdr_unregister_exception(u32 modid)
{
	return 0;
}
static inline int
rdr_register_module_ops(u64 coreid, struct rdr_module_ops_pub *ops,
			struct rdr_register_module_result *retinfo)
{
	return -1;
}
static inline int rdr_unregister_module_ops(u64 coreid)
{
	return 0;
}
static inline void rdr_system_error(u32 modid, u32 arg1, u32 arg2)
{
}
static inline unsigned int bbox_check_edition(void)
{
	return EDITION_USER;
}
static inline int rdr_wait_partition(const char *path, int timeouts, int mode)
{
	return 0;
}
static inline u64 get_32k_abs_timer_value(void)
{
	return 0;
}
static inline void rdr_bbox_unmap(const void *vaddr)
{
	return;
}
static inline u64 rdr_get_logsize(void)
{
	return 0;
}
static inline u32 rdr_get_lognum(void)
{
	return 0;
}
static inline char *rdr_get_timestamp(void)
{
	return NULL;
}
#endif

#endif /* End #define __BB_PUB_H__ */

/*
 * rdr_hisi_ap_exception_logsave.h
 *
 * Based on the RDR framework, adapt to the AP side to implement resource
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
#ifndef __RDR_HISI_AP_EXCEPTION_LOGSAVE_H__
#define __RDR_HISI_AP_EXCEPTION_LOGSAVE_H__

#include <linux/thread_info.h>
#include <linux/of.h>
#include <linux/notifier.h>

#define BUFFER_SIZE  128
#define MAX_MEMDUMP_NAME 16
#ifdef CONFIG_HISI_MNTN_PC
#define KDUMP_MAX_SIZE (8*1024*1024*1024LL)
#else
#define KDUMP_MAX_SIZE 0x80000000
#endif
#define MAX_STACK_TRACE_DEPTH 16
#define WAIT_PSTORE_PATH 60
#define LPM3_ADDR_SHIFT 2
#define BUILD_DISPLAY_ID  "ro.vendor.build.display.id"

struct memdump {
	char name[MAX_MEMDUMP_NAME];
	unsigned long base;
	unsigned long size;
};

struct sky_file_info {
	long fddst;
	long fdsrc;
	long seek_value;
	long seek_return;
};

void *memcpy_rdr(void *dest, const void *src, size_t count);
unsigned int get_ap_last_task_switch_from_dts(struct device *dev);
void rdr_regs_dump(void *dest, const void *src, size_t len);
int hisi_trace_hook_install(void);
void hisi_trace_hook_uninstall(void);

void save_kernel_dump(void *arg);
void save_logbuff_memory(void);
int save_exception_info(void *arg);
void save_hisiap_log(char *log_path, u32 modid);
void get_bbox_curtime_slice(void);
void rdr_hisiap_reset(u32 modid, u32 etype, u64 coreid);
void hisiap_callback(u32 argc, void *argv);
int rdr_copy_big_file_apend(const char *dst, const char *src);
int rdr_copy_file_apend(const char *dst, const char *src);
void save_pstore_info(const char *dst_dir_str);
void save_fastboot_log(const char *dst_dir_str);
int create_exception_dir(const char *exce_dir, char *dst_dir_str, u32 dst_dir_max_len);
int record_reason_task(void *arg);
int acpu_panic_loop_notify(struct notifier_block *nb, unsigned long event, void *buf);
int rdr_hisiap_panic_notify(struct notifier_block *nb, unsigned long event, void *buf);
int rdr_hisiap_die_notify(struct notifier_block *nb, unsigned long event, void *pReg);
int skp_save_kdump_file(const char *dst_str, const char *exce_dir);
#endif

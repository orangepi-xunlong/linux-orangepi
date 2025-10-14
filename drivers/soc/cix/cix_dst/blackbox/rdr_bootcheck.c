// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_bootcheck.c
 *
 * rdr startup abnormal monitoring
 *
 * Copyright (c) 2001-2019 Huawei Technologies Co., Ltd.
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

#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/soc/cix/boot_postcode.h>
#include "rdr_field.h"
#include "rdr_print.h"

#define RDR_NEED_SAVE_MEM 1
#define RDR_DONTNEED_SAVE_MEM 0
#define PSTORE_PATH "/sys/fs/pstore"

/*
 * check status of last reboot.
 * return
 * 0 dont need save.
 * 1 need save log.
 */
static int rdr_check_exceptionboot(struct rdr_exception_info_s *info)
{
	u32 temp_reboot_type;
	struct rdr_base_info_s *base = NULL;
	struct rdr_struct_s *tmpbb = NULL;

	if (!info) {
		BB_PN();
		return RDR_DONTNEED_SAVE_MEM;
	}

	temp_reboot_type = rdr_get_reboot_type();
	BB_PN("reboot_type = 0x%x\n", temp_reboot_type);
	/* If the exception type is normal, do not need to save log */
	if (temp_reboot_type < REBOOT_REASON_LABEL1 ||
	    (temp_reboot_type >= REBOOT_REASON_LABEL4)) {
		return RDR_DONTNEED_SAVE_MEM;
	}

	/* Save the default value of the log after reset */
	info->e_modid = RDR_MODID_AP_ABNORMAL_REBOOT;
	info->e_from_core = RDR_AP;
	info->e_notify_core_mask = RDR_AP;
	info->e_exce_type = temp_reboot_type;
	info->e_exce_subtype = rdr_get_exec_subtype_value();

	tmpbb = rdr_get_head(true);
	if (!tmpbb) {
		return RDR_DONTNEED_SAVE_MEM;
	}
	/* Get the bbox header struct */
	base = &(tmpbb->base_info);

	/* If the log is not saved before resetting, you need to save it again after the reset is started */
	if (base->start_flag != RDR_PROC_EXEC_DONE ||
	    base->savefile_flag != RDR_DUMP_LOG_DONE) {
		BB_ERR("ap error:start[%x],save done[%x]\n", base->start_flag,
		       base->savefile_flag);
		info->e_modid = BBOX_MODID_LAST_SAVE_NOT_DONE;
	} else
		return RDR_DONTNEED_SAVE_MEM;

	BB_ERR("reboot reason: %s-%s\n",
	       rdr_get_exception_type_name(info->e_exce_type),
	       rdr_get_exception_subtype_name(info->e_exce_type,
					      info->e_exce_subtype));
	return RDR_NEED_SAVE_MEM;
}

static void rdr_bootcheck_notify_dump(char *path,
				      struct rdr_exception_info_s *info)
{
	u64 result;

	if (!path || !info) {
		BB_ERR("paramtar is NULL\n");
		return;
	}

	BB_PN("create dump file path:[%s]\n", path);
	while (!rdr_module_is_register(info->e_notify_core_mask)) {
		BB_PN("wait module register. need[0x%llx]\n",
		      info->e_notify_core_mask);
		msleep(1000);
	}

	result = rdr_notify_module_dump(0, info->e_modid, info, path);
	BB_PN("rdr: notify [0x%llx] core dump data done\n", result);
}

static int rdr_save_history_log_back(void)
{
	struct rdr_exception_info_s temp;

	temp.e_from_core = RDR_AP;
	temp.e_reset_core_mask = RDR_AP;
	temp.e_exce_type = rdr_get_reboot_type();
	temp.e_exce_subtype = rdr_get_exec_subtype_value();

	return rdr_save_history_log(&temp, rdr_get_logdir_date(true),
				    DATATIME_MAXLEN, false,
				    get_last_bootup_postcode());
}

static int rdr_bootcheck_thread_body(void *arg)
{
	int cur_reboot_times;
	int ret;
	char *path;
	struct rdr_exception_info_s info;
	struct rdr_syserr_param_s p;
	struct rdr_struct_s *temp_pbb = NULL;
	unsigned int max_reboot_times = rdr_get_reboot_times();

	BB_PR_START();

	(void)rdr_dump_init();

	BB_PN("============wait for fs ready start =============\n");
	while (rdr_wait_partition(PSTORE_PATH, RDR_WAIT_PARTITION_TIME,
				  (S_IFDIR | S_IRUSR)) != 0) {
	}
	BB_PN("============wait for fs ready e n d =============\n");

	if (rdr_check_exceptionboot(&info) != RDR_NEED_SAVE_MEM) {
		BB_PN("need not save dump file when boot\n");
		goto end;
	}

	temp_pbb = rdr_get_head(true);
	if (temp_pbb->base_info.reserve == RDR_UNEXPECTED_REBOOT_MARK_ADDR) {
		cur_reboot_times = rdr_record_reboot_times2file();
		BB_PN("ap has reboot %d times\n", cur_reboot_times);
		if (max_reboot_times < (unsigned int)cur_reboot_times)
			rdr_reset_reboot_times(); /* reset the file of reboot_times */
	} else {
		rdr_reset_reboot_times();
	}

	p.modid = info.e_modid;
	p.arg1 = info.e_from_core;
	p.arg2 = info.e_exce_type;

	ret = rdr_saving_start(true);
	if (ret == -1) {
		BB_ERR("failed to create epath!\n");
		goto end;
	}

	path = rdr_get_logdir_path(true);
	if (IS_ERR_OR_NULL(path))
		goto end;

	rdr_bootcheck_notify_dump(path, &info);
	rdr_save_baseinfo(path, true);
	rdr_save_ramlog(path, RDR_SAVE_RAMLOG, true);
	rdr_save_cleartext(true);

	/* Create a new DONE file under the exception directory, indicating that the exception log is saved */
	bbox_save_done(path, BBOX_SAVE_STEP_DONE);
	rdr_save_history_log_back();
	/* File system sync to ensure read and write tasks are completed */
	rdr_sys_sync();

	BB_PN("saving data done\n");
	rdr_saving_end(true);

end:
	rdr_clear_last_head();
	BB_PR_END();
	return 0;
}

static int __init rdr_bootcheck_init(void)
{
	struct task_struct *rdr_bootcheck;

	rdr_bootcheck =
		kthread_run(rdr_bootcheck_thread_body, NULL, "bbox_bootcheck");
	if (rdr_bootcheck == NULL)
		BB_ERR("create thread rdr_bootcheck_thread faild\n");
	return 0;
}

late_initcall_sync(rdr_bootcheck_init);

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: vip_ldc_proc.h
 * Description:
 */

#ifndef _LDC_PROC_H_
#define _LDC_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "ldc_common.h"
#include "ldc_core.h"

int ldc_proc_init(void *shm);
int ldc_proc_remove(void);
void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, unsigned char top_id);
void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk);
void ldc_proc_record_job_start(struct ldc_job *job);
void ldc_proc_record_job_done(struct ldc_job *job);
void ldc_proc_commit_job(struct ldc_job *job);
void ldc_proc_update_timer_proc(bool suspend);

#ifdef __cplusplus
}
#endif

#endif /* _LDC_PROC_H_ */

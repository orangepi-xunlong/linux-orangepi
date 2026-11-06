/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File name: vip_ldc_proc.c
 * Description: video pipeline graphic distortion correction engine info
 */

#include "ldc_proc.h"
#include "ldc_debug.h"
#include "base_common.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define LDC_PROC_NAME "soph/ldc"

static struct ldc_proc_ctx *g_proc_ctx_ldc = NULL;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

//update dutyratio
static void _update_ldc_dutyratio(struct timer_list *timer);
DEFINE_TIMER(timer_proc, _update_ldc_dutyratio);
static atomic_t g_timer_added = ATOMIC_INIT(0);

static struct ldc_proc_ctx * ldc_get_proc_ctx(void)
{
	return g_proc_ctx_ldc;
}

static void _update_ldc_dutyratio(struct timer_list *timer)
{
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	u8 i;
	u32 duration;
	static u32 duration_long = 0;
	struct timespec64 cur_time;
	static struct timespec64 pre_time = {0};

	(void)(timer);

	ktime_get_ts64(&cur_time);
	duration = get_diff_in_us(pre_time, cur_time);
	duration_long += duration;
	pre_time = cur_time;

	if (duration > 2000000)
		goto FIRSTTIME;

	for (i = DEV_LDC_0; i < DEV_LDC_MAX; ++i) {
		proc_ctx->gdc_core_status[i].duty_ratio = (proc_ctx->gdc_core_status[i].core_all_hw_time * 100) / duration;
		proc_ctx->gdc_core_status[i].core_all_hw_time = 0;
	}

FIRSTTIME:
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
}

static int ldc_proc_show_tsk(struct seq_file *m, struct ldc_proc_ctx *pldcCtx, int idx)
{
	int  j;
	char c[32], c2[32];

	seq_printf(m, "\t\t%15s%15s%15s%15s%15s%15s%15s\n",
		"tskNo", "UseCoreID", "inSize","outSize", "State", "type", "HwTime(us)");

	for (j = 0; j < pldcCtx->job_info[idx].task_num; j++) {
		if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_RUNNING)
			strncpy(c, "RUNNING", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_DONE)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_CANCLE)
			strncpy(c, "CANCEL", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_ROT)
			strncpy(c2, "ROT", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_FISHEYE)
			strncpy(c2, "FISHEYE", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_AFFINE)
			strncpy(c2, "AFFINE", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_LDC)
			strncpy(c2, "LDC", sizeof(c2));
		else
			strncpy(c2, "UNKNOWN", sizeof(c2));

		seq_printf(m, "\t\t%14s%d%15d%15d%15d%15s%15s%15d\n",
			"#",
			j,
			pldcCtx->job_info[idx].tsk_info[j].top_id,
			pldcCtx->job_info[idx].tsk_info[j].in_size,
			pldcCtx->job_info[idx].tsk_info[j].out_size,
			c,
			c2,
			pldcCtx->job_info[idx].tsk_info[j].hw_time);
		pldcCtx->job_info[idx].hw_time += pldcCtx->job_info[idx].tsk_info[j].hw_time;
	}
	seq_puts(m, "\n----------------------------------------------------------------------------------------------------------------------------------------\n");

	return 0;
}

/*************************************************************************
 *	LDC proc functions
 *************************************************************************/
static int ldc_proc_show(struct seq_file *m, void *v)
{
	struct ldc_proc_ctx *pldcCtx = ldc_get_proc_ctx();
	unsigned long flags;
	int i, j, idx, total_handletime, total_hwTime, total_busyTime;
	char c[32];
	int idxs[LDC_PROC_JOB_INFO_NUM] = {0};
	char * dev_name[] = {"ldc0", "ldc1", "dwa0", "dwa1"};

	if (unlikely(!pldcCtx)) {
		seq_puts(m, "ldc proc ctx is NULL\n");
		return 0;
	}

	spin_lock_irqsave(&pldcCtx->lock, flags);
	idx = pldcCtx->job_idx - 1;
	spin_unlock_irqrestore(&pldcCtx->lock, flags);

	seq_printf(m, "\nModule: [LDC], Build Time[%s]\n", UTS_VERSION);

	if (idx < 0 || idx >= LDC_PROC_JOB_INFO_NUM) {
		TRACE_LDC(DBG_ERR, "invalid proc job idx[%d], out of range[%d]\n", idx, LDC_PROC_JOB_INFO_NUM);
		return 0;
	}

	// recent job info
	seq_puts(m, "\n------------------------------------------------------------RECENT JOB INFO--------------------------------------------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State", "CostTime(us)", "CoreID");
	seq_puts(m, "\n-----------------------------------------------------------------------------------------------------------------------------------\n");

	for (i = 0 ; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		if (!pldcCtx->job_info[idx].handle)
			break;

		memset(c, 0, sizeof(c));
		idxs[i] = idx;
		if (pldcCtx->job_info[idx].state == LDC_JOB_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d%15d\n",
			"#",
			i,
			(pldcCtx->job_info[idx].identity.mod_id == ID_BUTT) ?
				"USER" : MOD_STRING[pldcCtx->job_info[idx].identity.mod_id],
			pldcCtx->job_info[idx].identity.name,
			pldcCtx->job_info[idx].identity.sync_io,
			pldcCtx->job_info[idx].identity.id,
			pldcCtx->job_info[idx].task_num,
			c,
			pldcCtx->job_info[idx].cost_time,
			pldcCtx->job_info[idx].core_id);
		ldc_proc_show_tsk(m, pldcCtx, idx);

		idx = (--idx < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : idx;
	}

	// Max waste time job info
	seq_puts(m, "\n-------------------------------MAX WASTE TIME JOB INFO--------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State",
		"CostTime(us)");

	idx = i = pldcCtx->job_idx;
	for (j = 1; j < LDC_PROC_JOB_INFO_NUM; ++j) {
		i = (--i < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : i;
		if (!pldcCtx->job_info[i].handle)
			break;

		if (pldcCtx->job_info[i].cost_time > pldcCtx->job_info[idx].cost_time)
			idx = i;
	}

	for (i = 0 ; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		if (idxs[i] == idx)
			break;
	}

	if (pldcCtx->job_info[idx].handle) {
		memset(c, 0, sizeof(c));
		if (pldcCtx->job_info[idx].state == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			i,
			MOD_STRING[pldcCtx->job_info[idx].identity.mod_id],
			pldcCtx->job_info[idx].identity.name,
			pldcCtx->job_info[idx].identity.sync_io,
			pldcCtx->job_info[idx].identity.id,
			pldcCtx->job_info[idx].task_num,
			c,
			pldcCtx->job_info[idx].cost_time);

		ldc_proc_show_tsk(m, pldcCtx, idx);
	}

	// LDC job status
	seq_puts(m, "\n-------------------------------LDC JOB STATUS-----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n",
		"Success", "Fail", "Cancel", "BeginNum", "WaitNum", "ProcingNum");

	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->job_status.success,
				pldcCtx->job_status.fail,
				pldcCtx->job_status.cancel,
				pldcCtx->job_status.begin_num,
				pldcCtx->job_status.wait_num,
				pldcCtx->job_status.procing_num);

	// LDC task status
	seq_puts(m, "\n-------------------------------LDC TASK STATUS----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n", "Success", "Fail", "Cancel", "Begin", "Procing", "WaitNum");
	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->tsk_status.success,
				pldcCtx->tsk_status.fail,
				pldcCtx->tsk_status.cancel,
				pldcCtx->tsk_status.begin_num,
				pldcCtx->tsk_status.procing_num,
				pldcCtx->tsk_status.wait_num);

	// LDC interrupt status
	seq_puts(m, "\n-------------------------------LDC HW AVG STATUS-----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s%15s\n", "AVGNum", "TotalHwTm", "TotalBusyTm", "TotalCostTm", "AVGHwTm", "AVGBusyTm", "AVGCostTm(us)");

	total_handletime = total_hwTime = total_busyTime = 0;
	for (i = 0; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		total_hwTime += pldcCtx->job_info[i].hw_time;
		total_busyTime += pldcCtx->job_info[i].busy_time;
		total_handletime = total_hwTime + total_busyTime;
	}
	seq_printf(m, "%15d%15d%15d%15d%15d%15d%15d\n",
				LDC_PROC_JOB_INFO_NUM,
				total_hwTime,
				total_busyTime,
				total_handletime,
				total_hwTime / LDC_PROC_JOB_INFO_NUM,
				total_busyTime / LDC_PROC_JOB_INFO_NUM,
				total_handletime / LDC_PROC_JOB_INFO_NUM);

	// LDC call correction status
	seq_puts(m, "\n-------------------------------LDC CALL CORRECTION STATUS-----------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s\n", "TaskSuc", "TaskFail", "EndSuc", "EndFail", "CbCnt");
	seq_printf(m, "%15d%15d%15d%15d%15d\n",
				pldcCtx->fisheye_status.add_task_suc,
				pldcCtx->fisheye_status.add_task_fail,
				pldcCtx->fisheye_status.end_job_suc,
				pldcCtx->fisheye_status.end_job_fail,
				pldcCtx->fisheye_status.cb_cnt);


	// LDC core duty_ratio status
	seq_puts(m, "\n-------------------------------LDC HW DUTY RATIO STATUS-----------------\n");
	seq_printf(m, "%15s%15s%15s\n", "ID", "DEV", "DutyRatio");

	for (i = DEV_LDC_0; i < DEV_LDC_MAX; ++i) {
		seq_printf(m, "%14s%d%15s%15d\n",
			"#",
			i,
			dev_name[i],
			pldcCtx->gdc_core_status[i].duty_ratio);
	}

	return 0;
}

static int ldc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ldc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops ldc_proc_fops = {
	.proc_open = ldc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations ldc_proc_fops = {
	.owner = THIS_MODULE,
	.open = ldc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int ldc_handle_to_procIdx(struct ldc_job *job)
{
	int idx = -1;
	unsigned long flags;

	if (!job) {
		TRACE_LDC(DBG_ERR, "null job\n");
		return idx;
	}

	spin_lock_irqsave(&job->lock, flags);
	idx = job->proc_idx;
	spin_unlock_irqrestore(&job->lock, flags);

	return idx;
}

int ldc_proc_init(void *shm)
{
	int rc = 0, idx;
	unsigned long flags;

	if (proc_create_data(LDC_PROC_NAME, 0644, NULL, &ldc_proc_fops, NULL) == NULL) {
		pr_err("ldc proc creation failed\n");
		rc = -1;
	}

	g_proc_ctx_ldc = (struct ldc_proc_ctx *)shm;
	spin_lock_init(&g_proc_ctx_ldc->lock);

	spin_lock_irqsave(&g_proc_ctx_ldc->lock, flags);
	if (atomic_cmpxchg(&g_timer_added, 0, 1) == 0)
		add_timer(&timer_proc);

	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	spin_unlock_irqrestore(&g_proc_ctx_ldc->lock, flags);

	for (idx = 0; idx < LDC_PROC_JOB_INFO_NUM; ++idx)
		g_proc_ctx_ldc->job_info[idx].handle = 0;

	return rc;
}

int ldc_proc_remove(void)
{
	unsigned long flags;

	spin_lock_irqsave(&g_proc_ctx_ldc->lock, flags);
	if (atomic_cmpxchg(&g_timer_added, 1, 0) == 1)
		del_timer_sync(&timer_proc);

	spin_unlock_irqrestore(&g_proc_ctx_ldc->lock, flags);

	remove_proc_entry(LDC_PROC_NAME, NULL);
	g_proc_ctx_ldc = NULL;

	return 0;
}

void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, unsigned char top_id)
{
	int idx;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	idx = ldc_handle_to_procIdx(job);

	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);

		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = LDC_TASK_STATE_RUNNING;
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].top_id = top_id;
		proc_ctx->tsk_status.procing_num++;
		proc_ctx->tsk_status.wait_num =
			proc_ctx->tsk_status.begin_num - proc_ctx->tsk_status.procing_num;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk)
{
	int idx;
	unsigned long long end_time;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	enum ldc_task_state state;

	idx = ldc_handle_to_procIdx(job);

	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);
		end_time = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);

		state = atomic_read(&tsk->state);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = state;
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_time =
			(u32)(end_time - proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time);

		if (state == LDC_TASK_STATE_DONE)
			proc_ctx->tsk_status.success++;
		else if (state == LDC_TASK_STATE_CANCLE)
			proc_ctx->tsk_status.cancel++;
		else if (state == LDC_TASK_STATE_FAIL)
			proc_ctx->tsk_status.fail++;
		else
			TRACE_LDC(DBG_ERR, "invalid tsk id(%d) state (%d).\n", tsk->tsk_id, state);

	} else {
		TRACE_LDC(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}

}

void ldc_proc_record_job_start(struct ldc_job *job)
{
	int idx;
	struct timespec64 curTime;
	unsigned long long curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	idx = ldc_handle_to_procIdx(job);

	if (job && proc_ctx && idx >= 0 && idx < LDC_PROC_JOB_INFO_NUM) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);

		proc_ctx->job_info[idx].busy_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		proc_ctx->job_status.procing_num++;
		proc_ctx->job_status.wait_num =
			proc_ctx->job_status.begin_num - proc_ctx->job_status.procing_num;
		proc_ctx->job_info[idx].state = LDC_JOB_WORKING;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_job_done(struct ldc_job *job)
{
	int idx;
	struct timespec64 curTime;
	unsigned long long curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	enum ldc_job_state state;

	idx = ldc_handle_to_procIdx(job);

	if (job && proc_ctx && idx >= 0) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		state = atomic_read(&job->job_state);

		if (state == LDC_JOB_END)
			proc_ctx->job_status.success++;
		else if (state == LDC_JOB_CANCLE)
			proc_ctx->job_status.cancel++;
		else if (state == LDC_JOB_FAIL)
			proc_ctx->job_status.fail++;
		else
			TRACE_LDC(DBG_ERR, "invalid job state (%d).\n", state);

		proc_ctx->job_info[idx].state = state;
		proc_ctx->job_info[idx].cost_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		proc_ctx->job_info[idx].core_id = job->coreid;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_commit_job(struct ldc_job *job)
{
	unsigned short u16Idx, i;
	struct ldc_task *curelm;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;
	int tsk_num;

	if (job && proc_ctx) {
		tsk_num = atomic_read(&job->task_num);
		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->job_idx = (proc_ctx->job_idx >= LDC_PROC_JOB_INFO_NUM) ?
			0 : proc_ctx->job_idx;
		u16Idx = proc_ctx->job_idx;
		proc_ctx->job_idx++;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);

		spin_lock_irqsave(&job->lock, flags);
		job->proc_idx = u16Idx;
		spin_unlock_irqrestore(&job->lock, flags);

		TRACE_LDC(DBG_INFO, "proc ctx idx (%d)\n", u16Idx);

		memset(&proc_ctx->job_info[u16Idx], 0, sizeof(proc_ctx->job_info[u16Idx]));
		proc_ctx->job_info[u16Idx].handle = (u64)(uintptr_t)job;
		memcpy(&proc_ctx->job_info[u16Idx].identity, &job->identity, sizeof(job->identity));

		for (i = 0; i < tsk_num; i++) {
			spin_lock_irqsave(&job->lock, flags);
			curelm = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
			spin_unlock_irqrestore(&job->lock, flags);

			if (unlikely(!curelm)) {
				TRACE_LDC(DBG_ERR, "task index %d invalid\n", i);
				return;
			}
			proc_ctx->job_info[u16Idx].task_num++;
			proc_ctx->job_info[u16Idx].tsk_info[i].in_size =
				curelm->attr.img_in.video_frame.width *
				curelm->attr.img_in.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].out_size =
				curelm->attr.img_out.video_frame.width *
				curelm->attr.img_out.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].state = LDC_TASK_STATE_WAIT;
			proc_ctx->job_info[u16Idx].tsk_info[i].type = curelm->type;
			proc_ctx->tsk_status.begin_num++;
		}

		proc_ctx->job_info[u16Idx].state = LDC_JOB_WAIT;
		proc_ctx->job_status.begin_num++;

		ktime_get_ts64(&curTime);
		proc_ctx->job_info[u16Idx].submit_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
	}else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx invalid.\n");
	}
}

void ldc_proc_update_timer_proc(bool suspend)
{
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;

	spin_lock_irqsave(&proc_ctx->lock, flags);
	if (suspend) {
		if (atomic_cmpxchg(&g_timer_added, 1, 0) == 1)
			del_timer(&timer_proc);
	} else {
		if (atomic_cmpxchg(&g_timer_added, 0, 1) == 0)
			add_timer(&timer_proc);
		mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	}
	spin_unlock_irqrestore(&g_proc_ctx_ldc->lock, flags);
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_core.c
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

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/sched/debug.h>
#include <uapi/linux/sched/types.h>
#include "rdr_field.h"
#include "rdr_print.h"

struct ap_syserr_process_t {
	struct delayed_work dwork;
	struct rdr_exception_info_s *info;
	u32 modid;
	atomic_t running;
};

static struct semaphore rdr_sem;
static LIST_HEAD(g_rdr_syserr_list);
static DEFINE_SPINLOCK(g_rdr_syserr_list_lock);
static struct wakeup_source *blackbox_wl;
static struct rdr_exception_info_s *g_exce_info;
static struct ap_syserr_process_t g_ap_process;

static bool rdr_syserr_exist(u32 modid)
{
	struct rdr_syserr_param_s *p = NULL;
	bool exist = 0;

	spin_lock(&g_rdr_syserr_list_lock);
	list_for_each_entry(p, &g_rdr_syserr_list, syserr_list) {
		if (modid == p->modid) {
			exist = 1;
			BB_ERR("exception:[0x%x] disallow reentrant.  return\n",
			       modid);
			break;
		}
	}
	spin_unlock(&g_rdr_syserr_list_lock);

	return exist;
}

static void rdr_module_dump_post(struct rdr_exception_info_s *exce_info,
				 const char *path)
{
	u32 save_step = 0;

	save_step = (exce_info->e_reset_core_mask & RDR_AP) ?
			    BBOX_SAVE_STEP1 :
			    BBOX_SAVE_STEP_DONE;
	bbox_save_done(path, save_step);

	rdr_sys_sync();
}

static void rdr_module_dump(struct rdr_exception_info_s *p_exce_info,
			    char *path, u32 mod_id)
{
	u32 dump_mask;

	dump_mask = rdr_notify_module_dump(0, mod_id, p_exce_info, path);

	BB_PN("rdr_notify_module_dump done. return mask=[0x%x]\n", dump_mask);
	rdr_field_dumplog_done();

	if (dump_mask != 0)
		rdr_module_dump_post(p_exce_info, path);
	rdr_field_procexec_done();
	rdr_notify_module_dump(1, mod_id, p_exce_info, path);
}
static void rdr_dump_file(struct rdr_exception_info_s *p_exce_info, u32 mod_id)
{
	char *path = NULL;

	rdr_save_log(p_exce_info);
	/* notify to save the clear text */
	rdr_save_cleartext(false);
	path = rdr_get_logdir_path(false);
	if (!IS_ERR_OR_NULL(path))
		rdr_execption_callback(p_exce_info, mod_id, path);
}

static void rdr_dump(struct rdr_exception_info_s *p_exce_info, u32 mod_id)
{
	char *date = NULL;
	char *path = NULL;

	date = rdr_get_logdir_date(false);
	if (IS_ERR_OR_NULL(date))
		return;

	rdr_fill_edata(p_exce_info, date);

	path = rdr_get_logdir_path(false);
	if (!IS_ERR_OR_NULL(path))
		rdr_module_dump(p_exce_info, path, mod_id);
}

static void rdr_ap_exception_work(struct work_struct *work)
{
	struct ap_syserr_process_t *ap =
		container_of(work, struct ap_syserr_process_t, dwork.work);

	while (rdr_get_suspend_state()) {
		BB_PN("wait for suspend\n");
		msleep(50);
	}

	if (rdr_saving_start(false)) {
		BB_ERR("rdr_saving_start failed.  return\n");
		goto out;
	}
	BB_DBG("rdr_saving_start success\n");
	rdr_notify_module_dump(1, ap->modid, ap->info,
			       rdr_get_logdir_path(rdr_log_save_is_last()));
	rdr_dump_file(ap->info, ap->modid);
	rdr_saving_end(false);
out:
	rdr_notify_module_callback(ap->modid, ap->info);
	rdr_field_baseinfo_reinit();
	atomic_set(&g_ap_process.running, false);
}

static bool rdr_syserr_process_ap(u32 modid, u32 arg1, u32 arg2,
				  struct rdr_exception_info_s *info)
{
	char date[DATATIME_MAXLEN];
	int level = console_loglevel;

	BB_PR_START();
	if ((info->e_reboot_priority != RDR_REBOOT_NOW &&
	     !(info->e_reset_core_mask & RDR_AP)) &&
	    (in_atomic() || irqs_disabled() || in_irq()))
		return false;
	preempt_disable();
	console_loglevel = 7;
	rdr_save_args(modid, arg1, arg2);
	memset(date, 0, DATATIME_MAXLEN);
	(void)snprintf(date, DATATIME_MAXLEN, "%s-%08lld", rdr_get_timestamp(),
		       rdr_get_tick());

	rdr_fill_edata(info, date);
	(void)rdr_notify_module_dump(0, modid, info, NULL);
	rdr_notify_module_reset(modid, info);

	/*If the AP has not been reset, then save the log.*/
	preempt_enable();
	BB_PR_END();
	console_loglevel = level;
	if (atomic_read(&g_ap_process.running))
		return true;

	atomic_set(&g_ap_process.running, true);
	g_ap_process.info = info;
	g_ap_process.modid = modid;
	schedule_delayed_work(&g_ap_process.dwork, 0);
	return true;
}

static bool rdr_register_syserr(u32 modid, u32 arg1, u32 arg2)
{
	struct rdr_syserr_param_s *p = NULL;
	struct rdr_exception_info_s *p_exce_info = NULL;
	bool report = true;

	BB_PR_START();

	p_exce_info = rdr_get_exception_info(modid);
	if (p_exce_info == NULL) {
		BB_ERR("get exception failed, modid 0x%x\n", modid);
		return false;
	}

	(void)rdr_exception_trace_record_ap(p_exce_info->e_reset_core_mask,
					    p_exce_info->e_from_core,
					    p_exce_info->e_exce_type,
					    p_exce_info->e_exce_subtype);

	if (p_exce_info->e_from_core == RDR_AP) {
		/* process ap exception */
		if (rdr_syserr_process_ap(modid, arg1, arg2, p_exce_info))
			return false;
	}

	if (p_exce_info->e_reentrant == (u32)RDR_REENTRANT_DISALLOW) {
		report = !rdr_syserr_exist(modid);
	}
	if (!report)
		return false;

	p = kzalloc(sizeof(*p), GFP_ATOMIC);
	if (p == NULL) {
		BB_ERR("kzalloc rdr_syserr_param_s faild\n");
		return false;
	}

	p->modid = modid;
	p->arg1 = arg1;
	p->arg2 = arg2;

	BB_DBG("add syserr, modid=0x%x\n", modid);
	spin_lock(&g_rdr_syserr_list_lock);
	list_add_tail(&p->syserr_list, &g_rdr_syserr_list);
	spin_unlock(&g_rdr_syserr_list_lock);
	BB_PR_END();
	return true;
}

void rdr_system_error(u32 modid, u32 arg1, u32 arg2)
{
	char *modid_str = NULL;

	BB_PR_START();
	if (in_atomic() || irqs_disabled() || in_irq())
		BB_ERR("in atomic or irqs disabled or in irq\n");
	modid_str = blackbox_get_modid_str(modid);
	BB_ERR("blackbox receive exception modid is [0x%x][%s]!\n", modid,
	       modid_str);
	pr_emerg("%s", linux_banner);
	show_stack(current, NULL, KERN_EMERG);

	if (!rdr_init_done()) {
		BB_ERR("rdr init faild!\n");
		BB_PR_END();
		return;
	}

	if (rdr_register_syserr(modid, arg1, arg2))
		up(&rdr_sem);
	BB_PR_END();
}
EXPORT_SYMBOL(rdr_system_error);

struct rdr_exception_info_s *rdr_get_exce_info(void)
{
	return g_exce_info;
}

static int rdr_syserr_save_data(struct rdr_exception_info_s *p_exce_info,
				u32 mod_id)
{
	BB_PN("start saving data\n");
	if (rdr_saving_start(false)) {
		BB_ERR("rdr_saving_start failed.  return\n");
		return -1;
	}
	rdr_print_one_exc(p_exce_info);
	rdr_dump(p_exce_info, mod_id);
	rdr_dump_file(p_exce_info, mod_id);
	rdr_saving_end(false);
	rdr_notify_module_callback(mod_id, p_exce_info);
	BB_PN("saving data done\n");
	rdr_field_baseinfo_reinit();
	return 0;
}

static void rdr_syserr_process(struct rdr_syserr_param_s *p)
{
	u32 mod_id = p->modid;
	struct rdr_exception_info_s *p_exce_info = NULL;

	BB_PR_START();

	/* make sure that the task can not be interrupted by suspend. */
	__pm_stay_awake(blackbox_wl);
	rdr_save_args(p->modid, p->arg1, p->arg2);
	p_exce_info = rdr_get_exception_info(mod_id);

	while (1) {
		if (rdr_get_suspend_state()) {
			BB_PN("wait for suspend\n");
			msleep(50);
			continue;
		}
		break;
	}

	if (p_exce_info == NULL) {
		(void)rdr_save_history_log_for_undef_exception(p);
		__pm_relax(blackbox_wl);
		BB_ERR("get exception info faild.  return\n");
		return;
	}

	if (rdr_syserr_save_data(p_exce_info, mod_id)) {
		__pm_relax(blackbox_wl);
		return;
	}

	if (p_exce_info->e_upload_flag == (u32)RDR_UPLOAD_YES)
		BB_PN("rdr_upload log: done\n");

	BB_PN("rdr_notify_module_reset: start\n");
	rdr_notify_module_reset(mod_id, p_exce_info);
	BB_PN("rdr_notify_module_reset: done\n");

	__pm_relax(blackbox_wl);
	BB_PR_END();
}

bool rdr_syserr_list_empty(void)
{
	return list_empty(&g_rdr_syserr_list);
}

void rdr_syserr_list_print(void)
{
	struct rdr_syserr_param_s *e_cur = NULL;
	struct rdr_exception_info_s *p_exce_info = NULL;

	BB_PN("============start=============\n");
	BB_PN("empty? [%s]\n", rdr_syserr_list_empty() ? "true" : "false");
	spin_lock(&g_rdr_syserr_list_lock);
	list_for_each_entry(e_cur, &g_rdr_syserr_list, syserr_list) {
		p_exce_info = rdr_get_exception_info(e_cur->modid);
		if (p_exce_info == NULL) {
			BB_ERR("exception info is NULL\n");
			continue;
		}
		rdr_print_one_exc(p_exce_info);
		p_exce_info = NULL;
	}
	spin_unlock(&g_rdr_syserr_list_lock);
	BB_PN("============end=============\n");
}

static int rdr_exception_priority_process(void)
{
	struct rdr_syserr_param_s *e_cur = NULL;
	struct rdr_syserr_param_s *e_process = NULL;
	struct rdr_exception_info_s *p_exce_info = NULL;
	u32 e_priority = RDR_PPRI_MAX;

	spin_lock(&g_rdr_syserr_list_lock);
	list_for_each_entry(e_cur, &g_rdr_syserr_list, syserr_list) {
		p_exce_info = rdr_get_exception_info(e_cur->modid);
		if (unlikely(!p_exce_info)) {
			BB_ERR("rdr_get_exception_info fail\n");
			if (unlikely(!e_process))
				e_process = e_cur;
			continue;
		}
		if (p_exce_info->e_process_priority >= RDR_PPRI_MAX)
			BB_ERR("invalid prio[%u], current modid [0x%x]\n",
			       p_exce_info->e_process_priority, e_cur->modid);
		/* find the highest priority of all received exceptions in the list */
		if (p_exce_info->e_process_priority < e_priority) {
			BB_PN("current prio[%u], current modid [0x%x]\n",
			      p_exce_info->e_process_priority, e_cur->modid);
			e_process = e_cur;
			e_priority = p_exce_info->e_process_priority;
		}
	}

	if (unlikely(!e_process)) {
		BB_ERR("exception: NULL\n");
		spin_unlock(&g_rdr_syserr_list_lock);
		return -1;
	}

	list_del(&e_process->syserr_list);
	spin_unlock(&g_rdr_syserr_list_lock);

	rdr_syserr_process(e_process);
	kfree(e_process);

	return 0;
}

static int rdr_main_thread_body(void *arg)
{
	const long jiffies_time = (long)msecs_to_jiffies(5000);
	int ret;

	BB_PR_START();

	while (!kthread_should_stop()) {
		if (down_timeout(&rdr_sem, jiffies_time)) {
			if (rdr_syserr_list_empty())
				continue;
		}
		BB_DBG("enter into a new while =============\n");
		BB_DBG("============wait for fs ready start =============\n");
		while (rdr_wait_partition(PATH_MNTN_PARTITION,
					  RDR_WAIT_PARTITION_TIME,
					  (S_IFDIR | S_IWUSR)) != 0)
			;
		BB_DBG("============wait for fs ready end =============\n");
		while (!rdr_syserr_list_empty()) {
			ret = rdr_exception_priority_process();
			if (ret < 0)
				continue;
		}
	}
	BB_PR_END();
	return 0;
}

static bool init_done; /* default value is false */

static int rdr_func_init(struct platform_device *pdev)
{
	if (rdr_common_early_init(pdev) != 0) {
		BB_ERR("rdr_common_early_init faild\n");
		return -ENODEV;
	}

	if (rdr_common_init() != 0) {
		BB_ERR("rdr_common_init faild\n");
		return -EBUSY;
	}

	sema_init(&rdr_sem, 0);

	init_done = true;
	return 0;
}
bool rdr_init_done(void)
{
	return init_done;
}

static const struct of_device_id rdr_of_match[] = { { .compatible = "cix,dst" },
						    {} };

static int rdr_probe(struct platform_device *pdev)
{
	struct task_struct *rdr_main = NULL;
	struct sched_param param;

	BB_PR_START();

	atomic_set(&g_ap_process.running, false);
	INIT_DELAYED_WORK(&g_ap_process.dwork, rdr_ap_exception_work);
	if (rdr_func_init(pdev)) {
		BB_ERR("init environment faild\n");
		goto err;
	}

	blackbox_wl = wakeup_source_register(NULL, "blackbox");
	rdr_main = kthread_run(rdr_main_thread_body, NULL, "bbox_main");
	if (rdr_main == NULL) {
		BB_ERR("create thread rdr_main_thread faild\n");
		wakeup_source_unregister(blackbox_wl);
		goto err;
	}

	param.sched_priority = BBOX_RT_PRIORITY;
	if (sched_setscheduler(rdr_main, SCHED_FIFO, &param)) {
		BB_ERR("sched_setscheduler rdr_bootcheck_thread faild\n");
		kthread_stop(rdr_main);
		wakeup_source_unregister(blackbox_wl);
		goto err;
	}

	BB_PR_END();
	return 0;
err:
	BB_PR_END();
	return -1;
}

static int rdr_remove(struct platform_device *pdev)
{
	rdr_dump_exit();
	return 0;
}

static struct platform_driver rdr_driver = {
	.driver		= {
		.name			= "rdr driver",
		.of_match_table		= rdr_of_match,
	},
	.probe		= rdr_probe,
	.remove		= rdr_remove,
};

static s32 __init rdr_init(void)
{
	platform_driver_register(&rdr_driver);
	return 0;
}

static void __exit rdr_exit(void)
{
	platform_driver_unregister(&rdr_driver);
}

core_initcall(rdr_init);
module_exit(rdr_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("black box. kernel run data recorder");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");

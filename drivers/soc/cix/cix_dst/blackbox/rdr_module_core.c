// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_module_core.c
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
#include "rdr_field.h"
#include "rdr_print.h"

static LIST_HEAD(__rdr_module_list);
static DEFINE_SPINLOCK(__rdr_module_list_lock);
static u64 g_s_cur_regcore;

bool rdr_module_is_register(u64 coreid)
{
	return (coreid & g_s_cur_regcore) == coreid;
}

static void rdr_add_cur_regcore(u64 coreid)
{
	g_s_cur_regcore |= coreid;
}

static void __rdr_register_module_ops(struct rdr_module_ops_s *ops)
{
	struct rdr_module_ops_s *pos;
	bool inserted = false;
	unsigned long flags;

	BB_PR_START();
	if (!ops) {
		BB_ERR("invalid parameter ops\n");
		BB_PR_END();
		return;
	}

	spin_lock_irqsave(&__rdr_module_list_lock, flags);
	// Iterate each entry to find insertion point
	list_for_each_entry(pos, &__rdr_module_list, s_list) {
		if (ops->s_core_id > pos->s_core_id) {
			list_add(&ops->s_list, &pos->s_list);
			inserted = true;
			break;
		}
	}
	// If not inserted, add to tail
	if (!inserted)
		list_add_tail(&ops->s_list, &__rdr_module_list);

	spin_unlock_irqrestore(&__rdr_module_list_lock, flags);

	BB_DBG("Registered coreid [0x%llx]\n", ops->s_core_id);
	BB_PR_END();
}

static int rdr_get_module_info(u64 coreid,
			       struct rdr_register_module_result *retinfo)
{
	int ret, area;

	/*Determine if it is only for one coreid.*/
	if (RDR_CORE_IS_ERR(coreid))
		return -1;

	area = RDR_CORE_2_CORE_INDEX(coreid);
	ret = rdr_get_areainfo(area, retinfo);

	if (!ret)
		retinfo->nve = rdr_get_nve();
	return ret;
}

static int rdr_check_module_paras(u64 coreid, struct rdr_module_ops_pub *ops,
				  struct rdr_register_module_result *retinfo)
{
	if (retinfo == NULL) {
		BB_ERR("invalid parameter retinfo\n");
		return -EINVAL;
	}
	if (!rdr_init_done()) {
		BB_ERR("rdr init faild!\n");
		return -ENODEV;
	}
	if (ops == NULL) {
		BB_ERR("invalid parameter ops\n");
		return -EINVAL;
	}
	if (ops->ops_dump == NULL && ops->ops_reset == NULL) {
		BB_ERR("invalid  parameter ops.dump or reset\n");
		return -EINVAL;
	}
	if (rdr_module_is_register(coreid)) {
		BB_ERR("core_id 0x%llx already register\n", coreid);
		return -EEXIST;
	}
	return 0;
}

int rdr_register_module_ops(u64 coreid, struct rdr_module_ops_pub *ops,
			    struct rdr_register_module_result *retinfo)
{
	struct rdr_module_ops_s *pos = NULL;
	int ret;

	BB_PR_START();

	ret = rdr_check_module_paras(coreid, ops, retinfo);
	if (ret)
		goto out;

	pos = kzalloc(sizeof(*pos), GFP_ATOMIC);
	if (pos == NULL) {
		BB_ERR("kmalloc error, e_tpye_info\n");
		ret = -ENOMEM;
		goto out;
	}

	/* check modid & modid_end region */
	pos->s_core_id = coreid;
	pos->s_ops.ops_dump = ops->ops_dump;
	pos->s_ops.ops_dump_no_irq = ops->ops_dump_no_irq;
	pos->s_ops.ops_reset = ops->ops_reset;
	pos->s_ops.ops_callback = ops->ops_callback;

	__rdr_register_module_ops(pos);
	rdr_add_cur_regcore(coreid);

	ret = rdr_get_module_info(coreid, retinfo);
out:
	BB_PR_END();
	return ret;
}
EXPORT_SYMBOL(rdr_register_module_ops);

int rdr_unregister_module_ops(u64 coreid)
{
	struct rdr_module_ops_s *cur = NULL;
	struct rdr_module_ops_s *next = NULL;
	unsigned long lock_flag;

	BB_PR_START();
	spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	list_for_each_entry_safe(cur, next, &__rdr_module_list, s_list) {
		if (coreid == cur->s_core_id) {
			list_del(&cur->s_list);
			kfree(cur);
			g_s_cur_regcore &= ~coreid;
		}
	}
	spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
	BB_PR_END();
	return 0;
}
EXPORT_SYMBOL(rdr_unregister_module_ops);

static struct rdr_module_ops_s *rdr_get_module_ops(u64 coreid)
{
	struct rdr_module_ops_s *pos, *ret = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	list_for_each_entry(pos, &__rdr_module_list, s_list) {
		if (coreid == pos->s_core_id) {
			ret = pos;
			break;
		}
	}
	spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
	return ret;
}

static void handle_ap_reset(struct rdr_module_ops_s *ops, u32 modid,
			    struct rdr_exception_info_s *e_info)
{
	static bool reboot_later;
	u64 mask = e_info->e_reset_core_mask;
	bool reset_now = (e_info->e_reboot_priority == RDR_REBOOT_NOW);
	bool syserr_empty = rdr_syserr_list_empty();
	int reboot_times = 0;
	int max_reboot_times = (int)rdr_get_reboot_times();

	BB_PN("reboot priority[%s], need reboot flag[%s] syserr list[%s], coreid mask[0x%llx]\n",
	      reset_now ? "Now" : "Later",
	      reboot_later == true ? "true" : "false",
	      syserr_empty ? "empty" : "Non empty", mask & ops->s_core_id);

	if (reset_now || (reboot_later && syserr_empty)) {
		record_exce_type(e_info);
		goto reboot;
	}

	/*check if reset AP*/
	if (!(mask & RDR_AP))
		return;

	if (!syserr_empty) {
		record_exce_type(e_info);
		reboot_later = true;
		return;
	}

	record_exce_type(e_info);
reboot:
	rdr_record_reboot_times2mem();
	/*If AP, no exceptions, then record the file.*/
	if (e_info->e_from_core != RDR_AP) {
		reboot_times = rdr_record_reboot_times2file();
		BB_ERR("ap has reboot %d times\n", reboot_times);
		if (max_reboot_times < reboot_times)
			/* reset the file of reboot_times */
			rdr_reset_reboot_times();
	}
	rdr_flush_total_mem();
	BB_ERR("reboot now!\n");
	ops->s_ops.ops_reset(modid, e_info->e_exce_type, e_info->e_from_core);
}

static void handle_module_reset(struct rdr_module_ops_s *ops, u32 modid,
				struct rdr_exception_info_s *e_info)
{
	u64 mask = e_info->e_reset_core_mask;
	u64 core = ops->s_core_id;

	if (!(mask & core))
		return;
	if (IS_ERR_OR_NULL(ops->s_ops.ops_reset))
		return;
	BB_PN("reset [%s] start!\n", rdr_get_core_name_by_core(core));
	ops->s_ops.ops_reset(modid, e_info->e_exce_type, e_info->e_from_core);
	BB_PN("reset [%s] end!\n", rdr_get_core_name_by_core(core));
}

void rdr_notify_module_reset(u32 modid, struct rdr_exception_info_s *e_info)
{
	struct rdr_module_ops_s *ops = NULL;
	unsigned long lock_flag;
	u64 core;

	BB_PR_START();
	if (e_info == NULL) {
		BB_ERR("invalid  parameter e_info\n");
		BB_PR_END();
		return;
	}

	spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	list_for_each_entry(ops, &__rdr_module_list, s_list) {
		core = ops->s_core_id;
		BB_PN("core_id is [0x%llx]\n", core);
		/* special process for AP reset */
		if (core == RDR_AP) {
			spin_unlock_irqrestore(&__rdr_module_list_lock,
					       lock_flag);
			handle_ap_reset(ops, modid, e_info);
			spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
			continue;
		}
		spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
		handle_module_reset(ops, modid, e_info);
		spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	}
	spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
	BB_PR_END();
}

static u64 process_module_dump(int dumpmod, struct rdr_module_ops_s *ops,
			       u32 modid, u32 exce_type, u64 exce_core,
			       char *path)
{
	pfn_dump dump;
	if (IS_ERR_OR_NULL(ops))
		return 0;
	if (dumpmod)
		dump = ops->s_ops.ops_dump_no_irq;
	else
		dump = ops->s_ops.ops_dump;
	if (IS_ERR_OR_NULL(dump))
		return 0;

	BB_PN("dump module%s[%s] start\n", dumpmod ? " no irq " : " ",
	      rdr_get_core_name_by_core(ops->s_core_id));
	dump(modid, exce_type, exce_core, path);
	BB_PN("dump module%s[%s] end\n", dumpmod ? " no irq " : " ",
	      rdr_get_core_name_by_core(ops->s_core_id));
	return ops->s_core_id;
}

u64 rdr_notify_module_dump(int dumpmod, u32 modid,
			   struct rdr_exception_info_s *e_info, char *path)
{
	u64 mask;
	unsigned long lock_flag;
	u64 ret = 0;
	u64 core = e_info->e_from_core;
	struct rdr_module_ops_s *ops = NULL;

	BB_PR_START();
	if (e_info == NULL) {
		BB_ERR("invalid  parameter e_info\n");
		BB_PR_END();
		return ret;
	}

	/* first exception core dump */
	ops = rdr_get_module_ops(core);
	ret |= process_module_dump(dumpmod, ops, modid, e_info->e_exce_type,
				   core, path);

	mask = e_info->e_notify_core_mask & (~core);
	if (!mask)
		goto out;

	/* second notify core dump */
	spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	list_for_each_entry(ops, &__rdr_module_list, s_list) {
		if (!(mask & ops->s_core_id))
			continue;
		spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
		ret |= process_module_dump(dumpmod, ops, modid,
					   e_info->e_exce_type, core, path);
		spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	}
	spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);

out:
	BB_PR_END();
	return ret;
}

static void process_module_callback(struct rdr_module_ops_s *ops, u32 modid,
				    u32 exce_type, u64 exce_core)
{
	if (IS_ERR_OR_NULL(ops) || IS_ERR_OR_NULL(ops->s_ops.ops_callback))
		return;

	BB_PN("module callback [%s] start\n",
	      rdr_get_core_name_by_core(ops->s_core_id));
	ops->s_ops.ops_callback(modid, exce_type, exce_core);
	BB_PN("module callback [%s] end\n",
	      rdr_get_core_name_by_core(ops->s_core_id));
}

u64 rdr_notify_module_callback(u32 modid, struct rdr_exception_info_s *e_info)
{
	u64 mask;
	unsigned long lock_flag;
	u64 ret = 0;
	u64 core = e_info->e_from_core;
	struct rdr_module_ops_s *ops = NULL;

	BB_PR_START();
	if (e_info == NULL) {
		BB_ERR("invalid  parameter e_info\n");
		BB_PR_END();
		return ret;
	}

	mask = e_info->e_notify_core_mask & (~core);

	/* first notify core dump */
	spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	list_for_each_entry(ops, &__rdr_module_list, s_list) {
		if (!(mask & ops->s_core_id))
			continue;
		spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);
		process_module_callback(ops, modid, e_info->e_exce_type, core);
		spin_lock_irqsave(&__rdr_module_list_lock, lock_flag);
	}
	spin_unlock_irqrestore(&__rdr_module_list_lock, lock_flag);

	ops = rdr_get_module_ops(core);
	process_module_callback(ops, modid, e_info->e_exce_type, core);

	BB_PR_END();
	return ret;
}

void rdr_print_all_ops(void)
{
	int index = 1;
	struct rdr_module_ops_s *ops = NULL;

	BB_PR_START();
	spin_lock(&__rdr_module_list_lock);
	list_for_each_entry(ops, &__rdr_module_list, s_list) {
		BB_PN("==========[%.2d]-start==========\n", index);
		BB_PN(" core-id:        [0x%llx]\n", ops->s_core_id);
		BB_PN(" dump-fn:        [0x%pK]\n", ops->s_ops.ops_dump);
		BB_PN(" reset-fn:       [0x%pK]\n", ops->s_ops.ops_reset);
		BB_PN("==========[%.2d]-e n d==========\n", index);
		index++;
	}
	spin_unlock(&__rdr_module_list_lock);

	rdr_cleartext_print_ops();

	BB_PR_END();
}

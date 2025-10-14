// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_exception_core.c
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

#include <linux/soc/cix/dst_reboot_reason.h>
#include "rdr_inner.h"
#include "rdr_print.h"

#define GET_CALLBACK(cb) \
	((rdr_e_callback)((uintptr_t)(cb) & ~BBOX_CALLBACK_MASK))

static LIST_HEAD(__rdr_exception_list);
static spinlock_t __rdr_exception_list_lock;
static unsigned char g_lock_init;

static void execute_callback(rdr_e_callback cb, u32 argc, void *argv)
{
	rdr_e_callback r_cb = GET_CALLBACK(cb);

	if (IS_ERR_OR_NULL(r_cb))
		return;

	BB_PN("Invoking callback [%ps] for mod[0x%x]", cb, argc);
	r_cb(argc, argv);
}

void rdr_execption_callback(struct rdr_exception_info_s *p_exce_info, u32 argc,
			    void *argv)
{
	if (p_exce_info == NULL) {
		BB_ERR("invalid parameter p_exce_info\n");
		return;
	}

	/*Handle execption callback*/
	execute_callback(p_exce_info->e_callback, argc, argv);
}

static u32 rdr_check_modid(u32 modid, u32 modid_end)
{
	struct rdr_exception_info_s *e_type_info = NULL;

	spin_lock(&__rdr_exception_list_lock);
	list_for_each_entry(e_type_info, &__rdr_exception_list, e_list) {
		if ((modid >= e_type_info->e_modid &&
		     modid <= e_type_info->e_modid_end) ||
		    (modid_end >= e_type_info->e_modid &&
		     modid_end <= e_type_info->e_modid_end)) {
			spin_unlock(&__rdr_exception_list_lock);
			return modid;
		}
	}
	spin_unlock(&__rdr_exception_list_lock);

	return 0;
}

/*
 *   u32 modid,                exception id;
 *   if modid equal 0, will auto generation modid, and return it.
 *   u32 modid_end,            can register modid region. [modid~modid_end];
 *   need modid_end >= modid,
 *   if modid_end equal 0, will be register modid only,
 *   but modid & modid_end cant equal 0 at the same time.
 *   u32 process_priority,     exception process priority
 *   u32 reboot_priority,      exception reboot priority
 *   u32 save_log_mask,        need save log mask
 *   u32 notify_core_mask,     need notify other core mask
 *   u32 reset_core_mask,      need reset other core mask
 *   u32 reentrant,            whether to allow excption reentrant
 *   u32 from_core,            the core of happen excption
 *   char* from_module,        the module of happen excption
 *   char* desc,               the desc of happen excption
 *   bb_e_callback callback,   will be called when excption has processed.
 *   u32 reserve_u32;          reserve u32
 *   void* reserve_p           reserve void *
 */
/*
 * func args:
 *   struct rdr_exception_info_s* s_e_type
 *
 * return value      e_modid
 *   = 0 error
 *   >0 success
 */
u32 rdr_register_exception(struct rdr_exception_info_s *e)
{
	struct rdr_exception_info_s *e_type_info = NULL;
	u32 modid_end;

	if (!rdr_init_done()) {
		BB_ERR("rdr init faild!\n");
		return 0;
	}
	if (e == NULL) {
		BB_ERR("rdr_register_exception_type parameter is NULL!\n");
		return 0;
	}

	if (!g_lock_init) {
		spin_lock_init(&__rdr_exception_list_lock);
		g_lock_init = 1;
	}

	modid_end = e->e_modid_end;
	if (e->e_modid_end == 0 || e->e_modid_end < e->e_modid) {
		BB_ERR("modid[0x%x ~ 0x%x], but modid end is invalid.modify modid_end = [0x%x]\n",
		       e->e_modid, e->e_modid_end, e->e_modid);
		modid_end = e->e_modid;
	}
	BB_DBG("register modid [0x%x ~ 0x%x]\n", e->e_modid, modid_end);

	if (rdr_check_modid(e->e_modid, modid_end) != 0) {
		BB_PN("mod_id exist already\n");
		return 0;
	}

	e_type_info = kzalloc(sizeof(*e_type_info), GFP_ATOMIC);
	if (e_type_info == NULL) {
		BB_ERR("kmalloc failed for e_tpye_info\n");
		return 0;
	}

	/* check modid & modid_end region */
	memcpy(e_type_info, e, sizeof(*e_type_info));
	e_type_info->e_modid_end = modid_end;

	spin_lock(&__rdr_exception_list_lock);
	list_add_tail(&(e_type_info->e_list), &__rdr_exception_list);
	spin_unlock(&__rdr_exception_list_lock);

	BB_DBG("register exception succeed\n");

	return e_type_info->e_modid_end;
}
EXPORT_SYMBOL(rdr_register_exception);

/*
 * func args:
 *   u32 modid,          exception id;
 * return
 *   < 0 fail
 *   >=0 success
 */
int rdr_unregister_exception(u32 modid)
{
	struct rdr_exception_info_s *e_type_info, *next;

	BB_PR_START();
	spin_lock(&__rdr_exception_list_lock);
	list_for_each_entry_safe(e_type_info, next, &__rdr_exception_list,
				 e_list) {
		if (modid >= e_type_info->e_modid &&
		    e_type_info->e_modid_end >= modid) {
			BB_DBG("free exception [0x%x].", e_type_info->e_modid);
			list_del(&e_type_info->e_list);
			kfree(e_type_info);
		}
	}
	spin_unlock(&__rdr_exception_list_lock);

	BB_PR_END();

	return 0;
}
EXPORT_SYMBOL(rdr_unregister_exception);

/*
 * Description : Used by the rdr to record exceptions to the pmu
 */
void record_exce_type(const struct rdr_exception_info_s *einfo)
{
	if (!einfo) {
		BB_ERR("einfo is null\n");
		return;
	}
	set_reboot_reason(einfo->e_exce_type, einfo->e_exce_subtype);
}

/*
 * func args:
 *   u32 modid,           exception id;
 * return
 *   NULL fail
 *   other success
 *
 * pls. lock return pointer with __rdr_exception_list_lock.
 * because the pointer maybe free at any time.
 */
struct rdr_exception_info_s *rdr_get_exception_info(u32 modid)
{
	struct rdr_exception_info_s *e_type_info = NULL;
	unsigned long lock_flag;

	spin_lock_irqsave(&__rdr_exception_list_lock, lock_flag);
	list_for_each_entry(e_type_info, &__rdr_exception_list, e_list) {
		if (e_type_info->e_modid <= modid &&
		    e_type_info->e_modid_end >= modid) {
			spin_unlock_irqrestore(&__rdr_exception_list_lock,
					       lock_flag);
			return e_type_info;
		}
	}
	spin_unlock_irqrestore(&__rdr_exception_list_lock, lock_flag);

	return NULL;
}

void rdr_print_one_exc(struct rdr_exception_info_s *e)
{
	if (e == NULL) {
		BB_ERR("invalid  parameter e\n");
		return;
	}

	e->e_desc[STR_EXCEPTIONDESC_MAXLEN - 1] = '\0';
	e->e_from_module[MODULE_NAME_LEN - 1] = '\0';

	BB_PN(" modid:          [0x%x]\n", e->e_modid);
	BB_PN(" modid_end:      [0x%x]\n", e->e_modid_end);
	BB_PN(" process_pri:    [0x%x]\n", e->e_process_priority);
	BB_PN(" reboot_pri:     [0x%x]\n", e->e_reboot_priority);
	BB_PN(" notify_core_mk: [0x%llx]\n", e->e_notify_core_mask);
	BB_PN(" reset_core_mk:  [0x%llx]\n", e->e_reset_core_mask);
	BB_PN(" reentrant:      [0x%x]\n", e->e_reentrant);
	BB_PN(" exce_type:      [0x%x]\n", e->e_exce_type);
	BB_PN(" exce_subtype:   [0x%x]\n", e->e_exce_subtype);
	BB_PN(" from_core:      [0x%llx]\n", e->e_from_core);
	BB_PN(" from_module:    [%s]\n", e->e_from_module);
	BB_PN(" desc:           [%s]\n", e->e_desc);
	BB_PN(" callback:       [0x%pK]\n", e->e_callback);
	BB_PN(" reserve_u32:    [0x%x]\n", e->e_reserve_u32);
	BB_PN(" reserve_p:      [0x%pK]\n", e->e_reserve_p);
}

void rdr_print_all_exc(void)
{
	int index = 1;
	struct rdr_exception_info_s *e_type_info = NULL;

	BB_PR_START();
	spin_lock(&__rdr_exception_list_lock);
	list_for_each_entry(e_type_info, &__rdr_exception_list, e_list) {
		BB_PN("==========[%.2d]-start==========\n", index);
		rdr_print_one_exc(e_type_info);
		BB_PN("==========[%.2d]-e n d==========\n", index);
		index++;
	}
	spin_unlock(&__rdr_exception_list_lock);

	BB_PR_END();
}

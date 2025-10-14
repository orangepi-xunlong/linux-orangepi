// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_field_core.c
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

#include <linux/vmalloc.h>
#include "rdr_print.h"
#include "rdr_field.h"

static struct rdr_struct_s *g_rdr_head;
static struct rdr_struct_s *g_last_rdr_head;

struct rdr_struct_s *rdr_get_head(bool is_last)
{
	if (is_last)
		return g_last_rdr_head;
	return g_rdr_head;
}

void rdr_clear_last_head(void)
{
	if (g_last_rdr_head != NULL) {
		vfree(g_last_rdr_head);
		g_last_rdr_head = NULL;
	}
}

u32 rdr_total_mem_size(struct rdr_struct_s *data)
{
	return data->top_head.size;
}

int rdr_get_areainfo(int core_index, struct rdr_register_module_result *retinfo)
{
	struct bbox_mem mem;

	if (RDR_CORE_INDEX_IS_ERR(core_index) || (retinfo == NULL))
		return -1;

	if (rdr_safemem_get(&g_rdr_head->pool,
			    RDR_CORE_INDEX_2_CORE(core_index), &mem))
		return -1;
	retinfo->log_addr = mem.paddr;
	retinfo->log_len = mem.size;

	return 0;
}

static void rdr_field_baseinfo_init(void)
{
	BB_PR_START();
	g_rdr_head->base_info.modid = 0;
	g_rdr_head->base_info.arg1 = 0;
	g_rdr_head->base_info.arg2 = 0;
	g_rdr_head->base_info.e_core = 0;
	g_rdr_head->base_info.e_type = 0;
	g_rdr_head->base_info.e_subtype = 0;
	g_rdr_head->base_info.start_flag = 0;
	g_rdr_head->base_info.savefile_flag = 0;
	g_rdr_head->base_info.reboot_flag = 0;
	memset(g_rdr_head->base_info.e_module, 0, MODULE_NAME_LEN);
	memset(g_rdr_head->base_info.e_desc, 0, STR_EXCEPTIONDESC_MAXLEN);
	memset(g_rdr_head->base_info.datetime, 0, DATATIME_MAXLEN);

	g_rdr_head->cleartext_info.savefile_flag = 0;

	BB_PR_END();
}

void rdr_field_baseinfo_reinit(void)
{
	BB_PR_START();
	g_rdr_head->base_info.modid = 0;
	g_rdr_head->base_info.arg1 = 0;
	g_rdr_head->base_info.arg2 = 0;
	g_rdr_head->base_info.e_core = 0;
	g_rdr_head->base_info.e_type = 0;
	g_rdr_head->base_info.e_subtype = 0;
	g_rdr_head->base_info.start_flag = RDR_PROC_EXEC_START;
	g_rdr_head->base_info.savefile_flag = RDR_DUMP_LOG_START;

	memset(g_rdr_head->base_info.datetime, 0, DATATIME_MAXLEN);

	g_rdr_head->cleartext_info.savefile_flag = 0;

	BB_PR_END();
}

static int rdr_field_areainfo_init(struct rdr_area_data *data)
{
	int i, last, ret;
	struct bbox_mem mem;
	u64 size = 0;

	last = (int)data->value - 1;
	for (i = last; i > 0; i--) {
		ret = rdr_safemem_alloc(&g_rdr_head->pool,
					RDR_CORE_INDEX_2_CORE(i), data->data[i],
					&mem);
		if (ret)
			return ret;
	}
	/*The first*/
	size = mem.vaddr - g_rdr_head->pool.base_alloc_addr;
	ret = rdr_safemem_alloc(&g_rdr_head->pool, RDR_CORE_INDEX_2_CORE(0),
				size, &mem);

	return ret;
}

void rdr_cleartext_dumplog_done(void)
{
	g_rdr_head->cleartext_info.savefile_flag = 1;
}

void rdr_field_dumplog_done(void)
{
	g_rdr_head->base_info.savefile_flag = RDR_DUMP_LOG_DONE;
}

void rdr_field_procexec_done(void)
{
	g_rdr_head->base_info.start_flag = RDR_PROC_EXEC_DONE;
}

void rdr_field_reboot_done(void)
{
	g_rdr_head->base_info.reboot_flag = RDR_REBOOT_DONE;
}

static int rdr_field_top_init(void)
{
	int length, ret;

	BB_PR_START();

	g_rdr_head->top_head.magic = FILE_MAGIC;
	g_rdr_head->top_head.version = RDR_VERSION;
	g_rdr_head->top_head.area_number = RDR_CORE_MAX_INDEX;
	g_rdr_head->top_head.base_addr = rdr_reserved_mem().paddr;
	g_rdr_head->top_head.size = rdr_reserved_mem().size;

	rdr_get_builddatetime(g_rdr_head->top_head.build_time,
			      RDR_BUILD_DATE_TIME_LEN);
	length = strlen(RDR_PRODUCT) > RDR_PRODUCT_MAXLEN ? RDR_PRODUCT_MAXLEN :
							    strlen(RDR_PRODUCT);
	memcpy(g_rdr_head->top_head.product_name, RDR_PRODUCT, length);

	length = strlen(RDR_PRODUCT_VERSION) > RDR_PRODUCT_MAXLEN ?
			 RDR_PRODUCT_MAXLEN :
			 strlen(RDR_PRODUCT_VERSION);
	memcpy(g_rdr_head->top_head.product_version, RDR_PRODUCT_VERSION,
	       length);

	ret = rdr_bch_encode((void *)g_rdr_head,
			     offsetof(struct rdr_struct_s, ecc),
			     g_rdr_head->ecc, sizeof(g_rdr_head->ecc));

	BB_PR_END();
	return ret;
}

static int rdr_field_mempool_init(void)
{
	uint size = 0;
	bbox_mem mem;

	size = RDR_BASEINFO_SIZE - offsetof(struct rdr_struct_s, pool);
	mem.size = g_rdr_head->top_head.size - RDR_BASEINFO_SIZE;
	mem.vaddr = rdr_reserved_mem().vaddr + RDR_BASEINFO_SIZE;
	mem.paddr = rdr_reserved_mem().paddr + RDR_BASEINFO_SIZE;
	return rdr_safemem_pool_init(&g_rdr_head->pool, "rdr_mem", size, &mem,
				     false);
}

static int rdr_create_last_backup(void)
{
	int ret = 0;

	ret = rdr_bch_checkout((void *)g_rdr_head,
			       offsetof(struct rdr_struct_s, ecc),
			       g_rdr_head->ecc, sizeof(g_rdr_head->ecc));
	if (ret) {
		/*data err, don't backup*/
		BB_ERR("top info checkout err, %d\n", ret);
		return 0;
	}

	if (g_rdr_head->top_head.magic != FILE_MAGIC)
		return 0;

	g_last_rdr_head = vmalloc(rdr_reserved_mem().size);
	if (g_last_rdr_head == NULL) {
		BB_ERR("vmalloc g_tmp_pbb failed\n");
		rdr_bbox_unmap(g_rdr_head);
		g_rdr_head = NULL;
		return -1;
	}

	memcpy(g_last_rdr_head, g_rdr_head, rdr_reserved_mem().size);
	rdr_safemem_pool_reinit(&g_last_rdr_head->pool);
	rdr_show_base_info(true); /* show last info */
	return 0;
}

int rdr_field_init(struct rdr_area_data *data)
{
	int ret = 0;

	BB_PR_START();

	g_rdr_head = rdr_reserved_mem().vaddr;
	if (g_rdr_head == NULL) {
		BB_ERR("rdr_bbox_map g_pbb faild\n");
		ret = -1;
		goto out;
	}

	ret = rdr_create_last_backup();
	if (ret) {
		BB_ERR("rdr_create_last_backup failed\n");
		goto out;
	}
	/*
	 * if the power_up of phone is the first time,
	 * need clear bbox memory.
	 */
	if (rdr_get_reboot_type() == AP_S_COLDBOOT)
		memset(g_rdr_head, 0, rdr_reserved_mem().size);
	else
		memset(g_rdr_head, 0, RDR_BASEINFO_SIZE);

	// init buffer header
	ret = rdr_field_top_init();
	if (ret) {
		BB_ERR("rdr_field_top_init init failed\n");
		goto out;
	}
	rdr_field_baseinfo_init();
	ret = rdr_field_mempool_init();
	if (ret) {
		BB_ERR("rdr_mempoll init failed\n");
		goto out;
	}
	ret = rdr_field_areainfo_init(data);
	rdr_show_base_info(false);
	BB_PR_END();
out:
	return ret;
}

void rdr_field_exit(void)
{
}

void rdr_save_args(u32 modid, u32 arg1, u32 arg2)
{
	BB_PR_START();
	g_rdr_head->base_info.modid = modid;
	g_rdr_head->base_info.arg1 = arg1;
	g_rdr_head->base_info.arg2 = arg2;

	BB_PR_END();
}

void rdr_fill_edata(struct rdr_exception_info_s *e, const char *date)
{
	BB_PR_START();
	if ((e == NULL) || (date == NULL)) {
		BB_ERR("invalid  parameter!\n");
		BB_PR_END();
		return;
	}

	g_rdr_head->base_info.e_core = e->e_from_core;
	g_rdr_head->base_info.e_type = e->e_exce_type;
	g_rdr_head->base_info.e_subtype = e->e_exce_subtype;
	memcpy(g_rdr_head->base_info.datetime, date, DATATIME_MAXLEN);
	memcpy(g_rdr_head->base_info.e_module, e->e_from_module,
	       MODULE_NAME_LEN);
	memcpy(g_rdr_head->base_info.e_desc, e->e_desc,
	       STR_EXCEPTIONDESC_MAXLEN);
	BB_PR_END();
}

void rdr_show_base_info(bool is_last)
{
	struct rdr_struct_s *p = NULL;
	int index;
	struct bbox_mem mem;

	p = rdr_get_head(is_last);

	if (p == NULL)
		return;

	if (p->top_head.magic != FILE_MAGIC) {
		BB_PN("rdr_struct_s information is not initialized, no need to print it's content!\n");
		return;
	}

	p->base_info.datetime[DATATIME_MAXLEN - 1] = '\0';
	p->base_info.e_module[MODULE_NAME_LEN - 1] = '\0';
	p->base_info.e_desc[STR_EXCEPTIONDESC_MAXLEN - 1] = '\0';
	p->top_head.build_time[RDR_BUILD_DATE_TIME_LEN - 1] = '\0';

	BB_DBG("========= print baseinfo start =========\n");
	BB_DBG("modid        :[0x%x]\n", p->base_info.modid);
	BB_DBG("arg1         :[0x%x]\n", p->base_info.arg1);
	BB_DBG("arg2         :[0x%x]\n", p->base_info.arg2);
	BB_DBG("coreid       :[0x%x]\n", p->base_info.e_core);
	BB_DBG("reason       :[0x%x]\n", p->base_info.e_type);
	BB_DBG("subtype      :[0x%x]\n", p->base_info.e_subtype);
	BB_DBG("e data       :[%s]\n", p->base_info.datetime);
	BB_DBG("e module     :[%s]\n", p->base_info.e_module);
	BB_DBG("e desc       :[%s]\n", p->base_info.e_desc);
	BB_DBG("e start_flag :[%u]\n", p->base_info.start_flag);
	BB_DBG("e save_flag  :[%u]\n", p->base_info.savefile_flag);
	BB_DBG("e reserve      :[0x%x]\n", p->base_info.reserve);
	BB_DBG("========= print baseinfo e n d =========\n");

	BB_DBG("========= print top head start =========\n");
	BB_DBG("maigc        :[0x%x]\n", p->top_head.magic);
	BB_DBG("version      :[0x%x]\n", p->top_head.version);
	BB_DBG("area num     :[0x%x]\n", p->top_head.area_number);
	BB_DBG("buildtime    :[%s]\n", p->top_head.build_time);
	BB_DBG("========= print top head e n d =========\n");

	BB_DBG("========= print areainfo start =========\n");
	rdr_safemem_pool_show(&g_rdr_head->pool);
	for (index = 0; index < RDR_CORE_MAX_INDEX; index++) {
		if (rdr_safemem_get(&g_rdr_head->pool,
				    RDR_CORE_INDEX_2_CORE(index), &mem)) {
			BB_PN("area[%s] is no mem\n",
			      rdr_get_core_name_by_index(index));
			continue;
		}
		BB_DBG("area[%s] addr[0x%llx] size[0x%llx]\n",
		       rdr_get_core_name_by_index(index), mem.paddr, mem.size);
	}

	BB_DBG("========= print areainfo e n d =========\n");

	BB_DBG("========= print clear text start =========\n");
	BB_DBG("savefile_flag:[0x%x]\n", p->cleartext_info.savefile_flag);
	BB_DBG("========= print clear text e n d =========\n");
}

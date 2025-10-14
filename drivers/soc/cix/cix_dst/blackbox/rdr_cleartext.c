// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_cleartext.c
 *
 * blackbox cleartext. (kernel run data recorder clear text recording.)
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

#include <linux/syscalls.h>
#include <linux/dcache.h>
#include "rdr_print.h"
#include "rdr_field.h"

#define BUFFER_SIZE 128

static LIST_HEAD(rdr_cleartext_list);
static DEFINE_SPINLOCK(rdr_cleartext_lock);

void rdr_cleartext_print(struct file *fp, bool *error, const char *format, ...)
{
	va_list arglist;
	size_t len;
	char *buffer = NULL;
	int ret;

	if (IS_ERR_OR_NULL(fp) || IS_ERR_OR_NULL(error))
		return;

	/* stoping the next printing for the previous error printing */
	if (unlikely(*error == true))
		return;

	buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(buffer))
		return;

	/* get the string buffer for the next printing */
	va_start(arglist, format);
	ret = vsnprintf(buffer, PAGE_SIZE - 1, format, arglist);
	va_end(arglist);

	if (unlikely(ret <= 0)) {
		*error = true;
		BB_ERR("vsnprintf error ret %d\n", ret);
		kfree(buffer);
		return;
	}

	/* print the string buffer to the specified file descriptor */
	len = (size_t)ret;
	ret = kernel_write(fp, buffer, len, &(fp->f_pos));
	if (unlikely(ret != len)) {
		BB_ERR("write file exception with ret %d\n", ret);
		*error = true;
	}
	kfree(buffer);
}

struct file *bbox_cleartext_get_filep(const char *dir_path, char *file_name)
{
	struct file *fp = NULL;
	char path[PATH_MAXLEN];
	int flags, ret;

	if (IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(file_name)) {
		BB_ERR("invalid file path dir_path %pK file_name %pK!\n",
		       dir_path, file_name);
		return NULL;
	}

	if (rdr_create_dir(dir_path)) {
		BB_ERR("dir_path=%s, doesn't exsit or can't be created!\n",
		       dir_path);
		return NULL;
	}

	/* get the absolute file path string */
	ret = snprintf(path, PATH_MAXLEN - 1, "%s/%s", dir_path, file_name);
	if (unlikely(ret < 0)) {
		BB_ERR("snprintf_s ret %d!\n", ret);
		return NULL;
	}

	BB_DBG("path=%s\n", path);
	/* the file need to be trucated */
	flags = O_CREAT | O_RDWR | O_TRUNC;
	fp = filp_open(path, flags, FILE_LIMIT);
	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("create file %s err. fp=0x%pK\n", path, fp);
		return NULL;
	}

	vfs_llseek(fp, 0L, SEEK_END);
	return fp;
}

void bbox_cleartext_end_filep(struct file *fp)
{
	char *buffer = NULL;
	char *path = NULL;
	int ret;

	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("invalid fp %pK\n", fp);
		return;
	}

	buffer = kzalloc(0x1000, GFP_KERNEL);
	if (buffer)
		path = d_absolute_path(&fp->f_path, buffer, PATH_MAX);

	/* flush synchronize the specified file */
	vfs_fsync(fp, 0);

	/* close the specified file descriptor */
	filp_close(fp, NULL);

	/* According to the permission requirements, */
	/* hisi_logs directory and subdirectory groups adjust to root-system */
	ret = (int)rdr_chown(path, ROOT_UID, SYSTEM_GID, false);
	if (unlikely(ret))
		BB_ERR("chown %s uid [%d] gid [%d] failed err [%d]!\n", path,
		       ROOT_UID, SYSTEM_GID, ret);
	kfree(buffer);
}

/*
 * The clear text printing for the common header of reserved debug memory
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 *
 */
static int bbox_head_cleartext_print(const char *dir_path, u64 log_addr,
				     u32 log_len)
{
	struct rdr_struct_s *p = (struct rdr_struct_s *)(uintptr_t)log_addr;
	struct file *fp = NULL;
	bool error = false;

	if (IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(p)) {
		BB_ERR("error:dir_path 0x%pK log_addr 0x%pK\n", dir_path, p);
		return -1;
	}

	if (unlikely(log_len < sizeof(*p))) {
		BB_ERR("error:log_len %u sizeof(struct rdr_struct_s) %lu\n",
		       log_len, sizeof(*p));
		return -1;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "BBOX_HEAD_INFO.txt");
	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("error:fp 0x%pK\n", fp);
		return -1;
	}

	p->base_info.datetime[DATATIME_MAXLEN - 1] = '\0';
	p->base_info.e_module[MODULE_NAME_LEN - 1] = '\0';
	p->base_info.e_desc[STR_EXCEPTIONDESC_MAXLEN - 1] = '\0';
	p->top_head.build_time[RDR_BUILD_DATE_TIME_LEN - 1] = '\0';

	rdr_cleartext_print(fp, &error,
			    "========= print top head start =========\n");
	rdr_cleartext_print(fp, &error, "magic        :[0x%x]\n",
			    p->top_head.magic);
	rdr_cleartext_print(fp, &error, "version      :[0x%x]\n",
			    p->top_head.version);
	rdr_cleartext_print(fp, &error, "area num     :[0x%x]\n",
			    p->top_head.area_number);
	rdr_cleartext_print(fp, &error, "base_addr    :[0x%x]\n",
			    p->top_head.base_addr);
	rdr_cleartext_print(fp, &error, "size         :[0x%x]\n",
			    p->top_head.size);
	rdr_cleartext_print(fp, &error, "buildtime    :[%s]\n",
			    p->top_head.build_time);
	rdr_cleartext_print(fp, &error,
			    "========= print top head e n d =========\n");

	rdr_cleartext_print(fp, &error,
			    "========= print baseinfo start =========\n");
	rdr_cleartext_print(fp, &error, "modid        :[0x%x]\n",
			    p->base_info.modid);
	rdr_cleartext_print(fp, &error, "arg1         :[0x%x]\n",
			    p->base_info.arg1);
	rdr_cleartext_print(fp, &error, "arg2         :[0x%x]\n",
			    p->base_info.arg2);
	rdr_cleartext_print(fp, &error, "coreid       :[0x%x]\n",
			    p->base_info.e_core);
	rdr_cleartext_print(fp, &error, "reason       :[0x%x]\n",
			    p->base_info.e_type);
	rdr_cleartext_print(fp, &error, "subtype      :[0x%x]\n",
			    p->base_info.e_subtype);
	rdr_cleartext_print(fp, &error, "e data       :[%s]\n",
			    p->base_info.datetime);
	rdr_cleartext_print(fp, &error, "e module     :[%s]\n",
			    p->base_info.e_module);
	rdr_cleartext_print(fp, &error, "e desc       :[%s]\n",
			    p->base_info.e_desc);
	rdr_cleartext_print(fp, &error, "e start_flag :[0x%x]\n",
			    p->base_info.start_flag);
	rdr_cleartext_print(fp, &error, "e save_flag  :[0x%x]\n",
			    p->base_info.savefile_flag);
	rdr_cleartext_print(fp, &error, "e reboot_flag:[0x%x]\n",
			    p->base_info.reboot_flag);
	rdr_cleartext_print(fp, &error, "e reserve      :[0x%x]\n",
			    p->base_info.reserve);
	rdr_cleartext_print(fp, &error, "savefile_flag:[0x%x]\n",
			    p->cleartext_info.savefile_flag);
	rdr_cleartext_print(fp, &error,
			    "========= print baseinfo e n d =========\n");

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp);

	if (unlikely(error == true))
		return -1;

	return 0;
}

/*
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 *
 * func args:
 * @fp: the file descriptor of saved clear text
 * @error: to fast the cpu process when there is error happened
 *         before nowadays print, please refer the function
 *         bbox_head_cleartext_print to get the use of this parameter.
 * @e_from_core: the reboot exception comes from which core
 * @e_exce_type: the reboot exception type
 * @e_exce_subtype: the reboot exception subtype
 *
 * return value
 *
 */
static void _incorrect_reboot_reason_analysis(struct file *fp, bool *error,
					      u64 e_from_core, u32 e_exce_type,
					      u32 e_exce_subtype)
{
	rdr_cleartext_print(
		fp, error, "system exception core [%s], reason [%s:%s]\n",
		rdr_get_core_name_by_core(e_from_core),
		rdr_get_exception_type_name(e_exce_type),
		rdr_get_exception_subtype_name(e_exce_type, e_exce_subtype));
}

/*
 * in the case of reboot reason error reported, we must correct it to the real
 * reboot reason.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @exception: the corrected real reboot exception
 *
 * return value
 *
 */
void incorrect_reboot_reason_analysis(char *dir_path,
				      struct rdr_exception_info_s *exception)
{
	struct file *fp = NULL;
	bool error = false;

	if (IS_ERR_OR_NULL(dir_path) || IS_ERR_OR_NULL(exception)) {
		BB_ERR("error:dir_path 0x%pK exception 0x%pK\n", dir_path,
		       exception);
		return;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "incorrect_reboot_reason.txt");
	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("error:fp 0x%pK\n", fp);
		return;
	}

	rdr_cleartext_print(fp, &error, "<incorrect reboot reason>\n");
	_incorrect_reboot_reason_analysis(fp, &error, RDR_AP,
					  rdr_get_reboot_type(),
					  rdr_get_exec_subtype_value());

	rdr_cleartext_print(fp, &error, "<correct reboot reason>\n");
	_incorrect_reboot_reason_analysis(fp, &error, exception->e_from_core,
					  exception->e_exce_type,
					  exception->e_exce_subtype);

	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp);
}

/*
 * Get the registered clear text callback function
 * from the memory area id.
 *
 * func args:
 * @area_id: the specified core area id
 *
 * return value
 * NULL failure
 * otherwise success
 *
 */
static pfn_cleartext_ops rdr_get_cleartext_fn(u32 area_id, bool is_head)
{
	pfn_cleartext_ops ops = NULL;
	struct rdr_cleartext_ops_s *cur_ops = NULL;
	unsigned long lock_flag;
	u64 coreid;

	if (is_head)
		return bbox_head_cleartext_print;

	if (RDR_CORE_INDEX_IS_ERR(area_id))
		return NULL;
	coreid = (u64)RDR_CORE_INDEX_2_CORE(area_id);

	spin_lock_irqsave(&rdr_cleartext_lock, lock_flag);
	list_for_each_entry(cur_ops, &rdr_cleartext_list, s_list) {
		if (coreid == cur_ops->s_core_id) {
			ops = cur_ops->ops_cleartext;
			break;
		}
	}
	spin_unlock_irqrestore(&rdr_cleartext_lock, lock_flag);
	return ops;
}

/*
 * Get the start address of reserved memory and length for specified core first,
 * then transfer them to the correponding clear text callback function.
 * The registered clear text callback function is respobsible of the clear text
 * file output.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 * @area_id: the specified core area id
 *
 * return value
 *
 */
static void bbox_save_cleartext(const char *dir_path, u64 log_addr, u32 log_len,
				u32 area_id, bool is_head)
{
	pfn_cleartext_ops ops = NULL;
	char *core_path = NULL;
	int ret;

	BB_PR_START();

	ops = rdr_get_cleartext_fn(area_id, is_head);
	if (IS_ERR_OR_NULL(ops))
		return;

	core_path = (char *)dir_path;
	if (!(is_head || RDR_CORE_INDEX_IS_ERR(area_id))) {
		core_path = kzalloc(PATH_MAX, GFP_KERNEL);
		if (IS_ERR_OR_NULL(core_path)) {
			BB_ERR("%s core_path is NULL\n",
			       rdr_get_core_name_by_index(area_id));
			return;
		}
		snprintf(core_path, PATH_MAX, "%s/%s/", dir_path,
			 rdr_get_core_name_by_index(area_id));
	}

	BB_PN("call pfn_cleartext_ops %pS\n", ops);
	ret = ops(core_path, log_addr, log_len);
	if (unlikely(ret < 0))
		BB_ERR("call pfn_cleartext_ops %pS failed err [%d]!\n", ops,
		       ret);
	if (!(is_head || RDR_CORE_INDEX_IS_ERR(area_id)))
		kfree(core_path);
	BB_PR_END();
}

/*
 * Get the start address of reserved memory and length for each core first,
 * then transfer them to each registered clear text callback function who
 * come from surrounding core.
 * The registered clear text callback function is respobsible of the clear text
 * file output.
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @base_addr:the start address of reserved memory to save the debug info for each core
 *
 * return value
 *
 */
static void bbox_cleartext_proc(const char *path, const char *base_addr)
{
	char *addr = NULL, dir_path[PATH_MAXLEN];
	int ret;
	struct rdr_struct_s *r_head = NULL;
	bool is_last = rdr_log_save_is_last();
	uint modid, size;
	struct rdr_exception_info_s *p_exce_info = NULL;
	struct bbox_mem mem;
	u64 mask;

	if (!path || !base_addr) {
		BB_ERR("parameter invaild, please check\n");
		return;
	}

	ret = snprintf(dir_path, PATH_MAXLEN - 1, "%s%s", path, BBOX_SPLIT_BIN);
	if (unlikely(ret < 0)) {
		BB_ERR("snprintf_s error!\n");
		return;
	}

	r_head = (struct rdr_struct_s *)base_addr;
	BB_DBG("dir_path=%s base_addr=0x%px\n", dir_path, r_head);
	if (!is_last) {
		modid = r_head->base_info.modid;
		p_exce_info = rdr_get_exception_info(modid);
		if (!IS_ERR_OR_NULL(p_exce_info))
			mask = p_exce_info->e_from_core |
			       p_exce_info->e_notify_core_mask;
	}

	for (int i = 0; i < r_head->top_head.area_number; i++) {
		if (!is_last && !(RDR_CORE_INDEX_2_CORE(i) & mask)) {
			BB_DBG("%s is not in mask, skip\n",
			       rdr_get_core_name_by_index(i));
			continue;
		}
		if (rdr_safemem_get(&r_head->pool, RDR_CORE_INDEX_2_CORE(i),
				    &mem)) {
			BB_PN("%s is not mem, skip\n",
			      rdr_get_core_name_by_index(i));
			continue;
		}
		addr = (char *)base_addr + mem.paddr -
		       r_head->top_head.base_addr;
		size = mem.size;
		if (size)
			bbox_save_cleartext(dir_path, (u64)addr, size, i,
					    false);
	}

	bbox_save_cleartext(dir_path, (uintptr_t)r_head, RDR_BASEINFO_SIZE, 0,
			    true);

	rdr_sys_sync();
}

void rdr_save_cleartext(bool is_last)
{
	struct rdr_struct_s *r_head = NULL;
	char *path = NULL;

	r_head = rdr_get_head(is_last);
	if (IS_ERR_OR_NULL(r_head)) {
		BB_ERR("rdr_get_head failed\n");
		return;
	}

	path = rdr_get_logdir_path(is_last);
	if (IS_ERR_OR_NULL(path))
		return;

	if (!is_last) {
		bbox_cleartext_proc(path, (char *)r_head);
		/*
		 * save the cleartext dumplog over flag to avoid
		 * the same saving during the next reboot procedure
		 */
		rdr_cleartext_dumplog_done();
		return;
	}

	if (r_head->cleartext_info.savefile_flag == RDR_CLEARTEXT_LOG_DONE)
		return;
	bbox_cleartext_proc(path, (char *)r_head);
}

static void __rdr_register_cleartext_ops(struct rdr_cleartext_ops_s *ops)
{
	struct rdr_cleartext_ops_s *cur = NULL;
	struct rdr_cleartext_ops_s *next = NULL;

	if (!ops) {
		BB_ERR("invalid parameter\n");
		return;
	}

	list_for_each_entry_safe(cur, next, &rdr_cleartext_list, s_list) {
		if (ops->s_core_id < cur->s_core_id) {
			list_add_tail(&(ops->s_list), &cur->s_list);
			goto out;
		}
	}
	list_add_tail(&(ops->s_list), &rdr_cleartext_list);

out:
	BB_DBG("coreid is [0x%llx]\n", ops->s_core_id);
}

/*
 * func args:
 *   u64 core_id: the same with the parameter coreid of function rdr_register_module_ops
 *   pfn_cleartext_ops ops_fn: the function to write the content
 *       of reserved memory in clear text format
 *
 * return value
 *	< 0 error
 *	0 success
 */
int rdr_register_cleartext_ops(u64 coreid, pfn_cleartext_ops ops_fn)
{
	struct rdr_cleartext_ops_s *cur = NULL;
	unsigned long lock_flag;
	const int ret = -1;

	BB_PR_START();

	if (IS_ERR_OR_NULL(ops_fn)) {
		BB_ERR("invalid para ops_fn!\n");
		BB_PR_END();
		return ret;
	}

	if (unlikely(!rdr_init_done())) {
		BB_ERR("rdr init faild!\n");
		BB_PR_END();
		return ret;
	}

	spin_lock_irqsave(&rdr_cleartext_lock, lock_flag);
	list_for_each_entry(cur, &rdr_cleartext_list, s_list) {
		if (coreid == cur->s_core_id) {
			spin_unlock_irqrestore(&rdr_cleartext_lock, lock_flag);
			BB_ERR("core_id 0x%llx exist already\n", coreid);
			BB_PR_END();
			return ret;
		}
	}

	cur = kzalloc(sizeof(*cur), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(cur)) {
		spin_unlock_irqrestore(&rdr_cleartext_lock, lock_flag);
		BB_ERR("kzalloc error\n");
		BB_PR_END();
		return ret;
	}

	INIT_LIST_HEAD(&(cur->s_list));
	cur->s_core_id = coreid;
	cur->ops_cleartext = ops_fn;
	__rdr_register_cleartext_ops(cur);
	spin_unlock_irqrestore(&rdr_cleartext_lock, lock_flag);

	BB_DBG("%s", "rdr_register_cleartext_ops success\n");
	BB_PR_END();
	return 0;
}

/*
 * print registered clear text rollback function
 */
void rdr_cleartext_print_ops(void)
{
	struct rdr_cleartext_ops_s *cur = NULL;
	struct rdr_cleartext_ops_s *next = NULL;
	int index;

	BB_PR_START();
	index = 1;
	spin_lock(&rdr_cleartext_lock);
	list_for_each_entry_safe(cur, next, &rdr_cleartext_list, s_list) {
		BB_DBG("==========[%.2d]-start==========\n", index);
		BB_DBG(" cleartext-fn:   [0x%pK]\n", cur->ops_cleartext);
		BB_DBG("==========[%.2d]-e n d==========\n", index);
		index++;
	}
	spin_unlock(&rdr_cleartext_lock);

	BB_PR_END();
}

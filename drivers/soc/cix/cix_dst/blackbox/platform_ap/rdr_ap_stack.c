// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stack tracing support
 *
 * Copyright (C) 2025 CIX Ltd.
 */
#include <linux/cacheflush.h>
#include <linux/arm_sdei.h>
#include <linux/stddef.h>
#include <asm/stacktrace.h>
#include <sched/sched.h>
#include <rcu/rcu.h>
#include <linux/kallsyms.h>
#include <mntn_subtype_exception.h>
#include "include/rdr_ap_adapter.h"
#include "../rdr_print.h"
#include "../rdr_inner.h"

#define LASTSTACK_MAGIC 0xbabeface

#define DST_SET_LAST_STACK_MEMORY (0x14)

typedef struct frame_reg {
	uint64_t fp_next;
	uint64_t fp;
	uint64_t pc;
	int valid;
} arm64_frame_reg;

static struct bbox_mem g_stack;
static struct stack_head *head;

static bool arm64_fp_is_valid(uint64_t fp, uint64_t fp_next, uint64_t stackbase)
{
	if (fp < stackbase || fp > stackbase + THREAD_SIZE)
		return false;

	if (fp & 0xf)
		return false;

	if (fp < fp_next)
		return false;

	return true;
}

static bool arm64_search_stackframe(struct file *fp, u64 *stack_addr,
				    u64 stack_ori, struct stack_head *head)
{
	uint64_t *stack = stack_addr;
	uint64_t stack_base = stack_ori;
	uint32_t reg_num = THREAD_SIZE / 0x10;
	arm64_frame_reg *tmp_frame =
		kcalloc(reg_num, sizeof(arm64_frame_reg), GFP_KERNEL);
	uint32_t frame_cnt = 0, index;
	char name[KSYM_SYMBOL_LEN] = { 0 };
	ulong offset, pc;
	int current_index, start, end;
	bool err;

	/*Initial screening of FP and PC*/
	for (int index = reg_num - 1; index >= 0; index--) {
		uint64_t fp = stack[index * 2];
		uint64_t fp_next = stack_base + index * 0x10;

		if (arm64_fp_is_valid(stack[index * 2], fp_next, stack_base)) {
			tmp_frame[frame_cnt].fp = fp;
			tmp_frame[frame_cnt].fp_next = fp_next;
			tmp_frame[frame_cnt].pc = stack[index * 2 + 1];
			frame_cnt++;
		}
	}

	/*The second screening requires that the next FP equals the previous FP_NEXT*/
	for (int i = 0; i < frame_cnt; i++) {
		uint64_t fp_next = tmp_frame[i].fp_next;

		for (int j = i + 1; j < frame_cnt; j++)
			if (tmp_frame[j].fp == fp_next) {
				tmp_frame[i].valid = 1;
				break;
			}
	}

	/*Find the starting FP*/
	start = 0;
	end = 0;
	for (int i = 0; i < frame_cnt; i++) {
		if (tmp_frame[i].valid) {
			start = i;
			break;
		}
	}

	/*Find the last consecutive FP*/
	current_index = start;
	for (int i = start + 1; i < frame_cnt; i++) {
		if (!tmp_frame[i].valid)
			continue;

		if (tmp_frame[current_index].fp_next != tmp_frame[i].fp)
			break;

		current_index = i;
	}

	/*Find the Top of the Stack*/
	end = current_index;
	for (int i = current_index + 1; i < frame_cnt; i++) {
		if (tmp_frame[i].valid)
			break;

		if (tmp_frame[i].fp == tmp_frame[end].fp_next) {
			end = i;
			break;
		}
	}

	rdr_cleartext_print(fp, &err, "maybe frame:\n");
	index = 0;
	for (int i = end; i >= start; i--) {
		if (!tmp_frame[i].valid && i != end)
			continue;
		memset(name, 0, sizeof(name));
		offset = 0;
		pc = ptrauth_strip_kernel_insn_pac(tmp_frame[i].pc) - 4 -
		     head->kaslr + kaslr_offset();
		kallsyms_lookup(pc, NULL, &offset, NULL, name);
		rdr_cleartext_print(fp, &err, "\tframe[%d]: 0x%lx[%s+0x%lx]\n",
				    index, pc, name, offset);
		index++;
	}

	rdr_cleartext_print(fp, &err, "all symbols:\n", index, pc, name,
			    offset);
	index = 0;
	for (int i = frame_cnt - 1; i >= 0; i--) {
		if (!tmp_frame[i].valid)
			continue;
		memset(name, 0, sizeof(name));
		offset = 0;
		pc = ptrauth_strip_kernel_insn_pac(tmp_frame[i].pc) - 4 -
		     head->kaslr + kaslr_offset();
		kallsyms_lookup(pc, NULL, &offset, NULL, name);
		rdr_cleartext_print(fp, &err,
				    "\tsymbols[%d]: 0x%lx[%s+0x%lx]\n", index,
				    pc, name, offset);
		index++;
	}

	kfree(tmp_frame);

	return true;
}

static bool check_laststack_cleartext(void)
{
	u32 type, subtype;

	if (!rdr_log_save_is_last())
		return false;

	type = rdr_get_reboot_type();
	subtype = rdr_get_exec_subtype_value();
	if (type == AP_PANIC)
		if (subtype == AP_PANIC_HARDLOCKUP ||
		    subtype == AP_PANIC_RCUSTALL)
			return true;

	if (type == SE_REBOOT || type == PM_REBOOT)
		return true;

	if (type == AP_AWDT)
		return true;
	return false;
}

int ap_laststack_cleartext(const char *dir_path, u64 log_addr, u32 log_len)
{
	struct ap_eh_root *head = (struct ap_eh_root *)log_addr;
	struct stack_head *s_head = NULL;
	struct file *fp;
	struct bbox_mem mem;
	bool err;

	if (!check_laststack_cleartext())
		return 0;

	if (IS_ERR_OR_NULL(head))
		return -1;

	if (rdr_safemem_get(&head->pool, MEMID_STACK, &mem))
		return -1;
	s_head = get_addr_from_root(head, mem.vaddr);
	if (IS_ERR_OR_NULL(s_head))
		return -1;

	fp = bbox_cleartext_get_filep(dir_path, "laststack");
	if (IS_ERR_OR_NULL(fp))
		return -1;

	rdr_cleartext_print(fp, &err, "head: 0x%lx\n", (uint64_t)s_head);
	rdr_cleartext_print(fp, &err, "stack_size: 0x%lx\n", s_head->size);
	rdr_cleartext_print(fp, &err, "page_size: 0x%lx\n", s_head->page_size);
	rdr_cleartext_print(fp, &err, "kaslr: 0x%lx\n", s_head->kaslr);
	rdr_cleartext_print(fp, &err, "cpu_num: %d\n", s_head->cpu_num);

	for (int i = 0; i < s_head->cpu_num; i++) {
		rdr_cleartext_print(fp, &err, "cpu[%d] comm: %s\n", i,
				    s_head->stack[i].comm);
		arm64_search_stackframe(fp, (void *)s_head->stack[i].data,
					s_head->stack[i].vaddr, s_head);
		rdr_cleartext_print(fp, &err, "\n");
	}

	bbox_cleartext_end_filep(fp);

	return 0;
}

int stack_dump_init(struct platform_device *pdev, struct rdr_safemem_pool *pool)
{
	u64 size = sizeof(struct stack_head);
	struct bbox_mem *info = &g_stack;

	if (rdr_safemem_alloc(pool, MEMID_STACK, size, info))
		return -1;

	memset(info->vaddr, 0, size);
	head = info->vaddr;
	head->kaslr = kaslr_offset();
	head->size = THREAD_SIZE;
	head->page_size = PAGE_SIZE;
	head->stext = (u64)_stext;
	head->etext = (u64)_etext;
	head->cpu_num = num_online_cpus();
	head->stack_size = sizeof(struct stack_data);
	head->stack_offset = offsetof(struct stack_head, stack);
	for (int i = 0; i < MAX_CPU_NUM; i++) {
		head->stack[i].comm_offset = offsetof(struct stack_data, comm);
		head->stack[i].data_offset = offsetof(struct stack_data, data);
		head->stack[i].pa_offset = offsetof(struct stack_data, pa);
	}
	dcache_clean_poc((u64)head, (u64)head + size);

	dst_sec_call(DST_SET_LAST_STACK_MEMORY, LASTSTACK_MAGIC,
		     (u64)info->paddr, 0);
	BB_DBG("stack save vaddr: %px, paddr: 0x%llx\n", info->vaddr,
	       info->paddr);

	return 0;
}

void stack_last_task_update(int cpu, struct task_struct *task)
{
	struct stack_data *stack;
	struct vm_struct *vm;

	if (IS_ERR_OR_NULL(head))
		return;

	stack = &head->stack[cpu];
	strncpy(stack->comm, task->comm, sizeof(task->comm) - 1);
	stack->comm[TASK_COMM_LEN - 1] = '\0';

	vm = task->stack_vm_area;
	if (IS_ERR_OR_NULL(vm)) {
		// vm = find_vm_area(task->stack);
		return;
	}
	stack->vaddr = (u64)task->stack;
	for (int i = 0; i < THREAD_SIZE / PAGE_SIZE; i++)
		stack->pa[i] = page_to_phys(vm->pages[i]);
}

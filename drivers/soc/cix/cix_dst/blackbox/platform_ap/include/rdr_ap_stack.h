/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __RDR_AP_STACK_H__
#define __RDR_AP_STACK_H__

#include <linux/platform_device.h>
#include <linux/soc/cix/rdr_pub.h>

#define MAX_CPU_NUM 24

struct stack_data {
	u64 vaddr;
	u32 comm_offset;
	u32 pa_offset;
	u32 data_offset;
	char comm[TASK_COMM_LEN];
	u64 pa[THREAD_SIZE / PAGE_SIZE];
	u8 data[THREAD_SIZE];
};

struct stack_head {
	u64 size;
	u64 page_size;
	u64 kaslr;
	u64 stext;
	u64 etext;
	u32 cpu_num;
	u32 stack_size;
	u32 stack_offset;
	struct stack_data stack[MAX_CPU_NUM];
};

int stack_dump_init(struct platform_device *pdev, struct rdr_safemem_pool *pool);
void stack_last_task_update(int cpu, struct task_struct *task);
int ap_laststack_cleartext(const char *dir_path, u64 log_addr, u32 log_len);

#endif

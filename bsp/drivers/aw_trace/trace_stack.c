// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner trace stack
 *
 * Copyright (C) 2022 Allwinner.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/splice.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/stacktrace.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>

#define MAX_FUNC_SIZE 64
static LIST_HEAD(kprobe_list);
struct aw_ktrace {
	struct kprobe kp;
	struct list_head list;
	char *symbol_name;
	unsigned long __percpu *cycles;
};

unsigned long entries[2048];
char stackbuf[4096];

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	struct aw_ktrace *atrace;
	unsigned long *cycles;

	atrace = container_of(p, struct aw_ktrace, kp);
	cycles = this_cpu_ptr(atrace->cycles);
	*cycles = get_cycles();

	return 0;
}

static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
	int num_entries = 0, i;
	int len = 0;
	unsigned long *cycles;
	struct aw_ktrace *atrace = container_of(p, struct aw_ktrace, kp);

	cycles = this_cpu_ptr(atrace->cycles);
	if (*cycles)
		*cycles = get_cycles() - *cycles;

	num_entries = stack_trace_save(entries, ARRAY_SIZE(entries), 0);

	for (i = 0; i < num_entries; i++)
		len += snprintf(stackbuf + len, 64, "          %pS\n", entries[i]);
	trace_printk("%s is consume %u us,Call stack:\n%s", p->symbol_name, *cycles / 24, stackbuf);
	*cycles = 0;
}

static ssize_t trace_function_write(struct file *file, const char __user *ubuf,
				    size_t count, loff_t *ppos)
{
	struct aw_ktrace *atrace = NULL;
	struct kprobe *kp = NULL;
	char buf[64];
	int ret = -EFAULT;

	atrace = kmalloc(sizeof(*atrace), GFP_KERNEL);
	kp = &atrace->kp;
	atrace->symbol_name = kzalloc(MAX_FUNC_SIZE, GFP_KERNEL);
	atrace->cycles = alloc_percpu(unsigned long);

	if (count > MAX_FUNC_SIZE)
		count = MAX_FUNC_SIZE;

	memset(buf, 0, MAX_FUNC_SIZE);
	if (copy_from_user(buf, ubuf, (count - 1)))
		return -EFAULT;

	buf[count] = '\0';

	memcpy(atrace->symbol_name, buf, MAX_FUNC_SIZE);
	kp->pre_handler = handler_pre;
	kp->post_handler = handler_post;
	kp->symbol_name = atrace->symbol_name;
	ret = register_kprobe(kp);
	if (ret < 0)
		pr_err("register kprobe:%s failed, ret:%d\n", kp->symbol_name, ret);
	INIT_LIST_HEAD(&atrace->list);
	list_add_tail(&atrace->list, &kprobe_list);
	return count;
}

static int trace_function_show(struct seq_file *m, void *v)
{
	struct aw_ktrace *atrace;

	list_for_each_entry(atrace, &kprobe_list, list)
		seq_printf(m, "%s\n", atrace->symbol_name);

	return 0;
}

static int trace_function_open(struct inode *inode, struct file *file)
{
	return single_open(file, trace_function_show, inode->i_private);
}

static const struct proc_ops trace_function_ops = {
	.proc_open = trace_function_open,
	.proc_read = seq_read,
	.proc_write = trace_function_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static ssize_t remove_function_write(struct file *file, const char __user *ubuf,
				     size_t count, loff_t *ppos)
{
	struct aw_ktrace *atrace;
	char buf[MAX_FUNC_SIZE];

	if (count > MAX_FUNC_SIZE)
		count = MAX_FUNC_SIZE;

	memset(buf, 0, MAX_FUNC_SIZE);
	if (copy_from_user(buf, ubuf, (count - 1)))
		return -EFAULT;

	buf[count] = '\0';

	list_for_each_entry(atrace, &kprobe_list, list) {
		if (!strcmp(buf, atrace->symbol_name)) {
			unregister_kprobe(&atrace->kp);
			kfree(atrace->symbol_name);
			list_del(&atrace->list);
			kfree(atrace);
			break;
		}
	}
	return count;
}

static const struct proc_ops remove_function_ops = {
	.proc_write = remove_function_write,
};

struct proc_dir_entry *parent_dir;
static int __init trace_stack_init(void)
{
	parent_dir = proc_mkdir("trace_stack", NULL);

	if (!parent_dir) {
		pr_err("mkdir aw_trace failed\n");
		goto end;
	}
	pr_info("create aw_trace success!,0x%llx\n", parent_dir);

	if (!proc_create("trace_function", 0600, parent_dir, &trace_function_ops)) {
		pr_err("mkdir trace_function failed\n");
		goto remove_dir;
	}
	pr_info("create trace_func success!\n");

	if (!proc_create("remove_function", 0600, parent_dir, &remove_function_ops)) {
		pr_err("mkdir trace_function failed\n");
		goto remove_dir;
	}

	pr_info("trace stack registered\n");

	return 0;
remove_dir:
	remove_proc_subtree("trace_stack", NULL);
end:
	return -1;
}

static void __exit trace_stack_exit(void)
{
	if (parent_dir)
		remove_proc_subtree("trace_stack", NULL);

	pr_info("trace stack unregistered\n");
}

module_init(trace_stack_init);
module_exit(trace_stack_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("henryli");
MODULE_DESCRIPTION("Aw Trace Stack");
MODULE_VERSION("1.0.0");

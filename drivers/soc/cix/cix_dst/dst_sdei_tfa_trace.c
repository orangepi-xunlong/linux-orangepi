// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#include <linux/memblock.h>
#include <linux/debugfs.h>
#include <linux/arm-smccc.h>
#include <linux/cacheflush.h>
#include <linux/soc/cix/rdr_platform.h>
#include "dst_print.h"

#define DST_SET_OS_MEM_SIZE (0x12)
#define DST_SET_TFA_TRACE_MEMORY (0x13)
#define DST_EXCEPTION_DEBUG (0xff)

#ifdef CONFIG_PLAT_SDEI_EXCEPTIONS_TEST
static int sdei_debug_show(struct seq_file *m, void *v)
{
	seq_printf(m,
		   "1: echo 0 > /sys/kernel/debug/tf-a/debug, ATF exception\n");
	return 0;
}

static int sdei_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, sdei_debug_show, inode->i_private);
}

static ssize_t sdei_debug_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	if (count) {
		char c;

		if (get_user(c, buf))
			return -EFAULT;
		dst_sec_call(DST_EXCEPTION_DEBUG, c, 0, 0);
	}

	return count;
}

static const struct file_operations sdei_debug_ops = {
	.open = sdei_debug_open,
	.write = sdei_debug_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static void sdei_debug_init(void)
{
	struct dentry *sdei_debug_root;

	sdei_debug_root = debugfs_create_dir("sdei", NULL);
	debugfs_create_file("debug", S_IWUSR, sdei_debug_root, NULL,
			    &sdei_debug_ops);
}
#endif

static int tfa_trace_dump(void *dump_addr, unsigned int size)
{
	/*invalid cache*/
	dcache_inval_poc((unsigned long)dump_addr,
			 (unsigned long)((char *)dump_addr + size));

	return 0;
}

static int __init dst_tfa_trace_init(void)
{
	unsigned char *virt_addr;
	uintptr_t phy_addr;
	u32 size, base;
	u64 os_size = 0;

	DST_PR_START();
	if (get_module_dump_mem_addr(MODU_TFA, &virt_addr, &size)) {
		DST_ERR("get module memory failed.\n");
		return 0;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) +
		   ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_PN("phys memory address=0x%lx, size=0x%x\n", phy_addr, size);

	if (dst_sec_call(DST_SET_TFA_TRACE_MEMORY, size, (u64)phy_addr, 0)) {
		DST_ERR("set sdei tfa trace memory failed.\n");
		return 0;
	}
	register_module_dump_mem_func(tfa_trace_dump, "tfa", MODU_TFA);

#ifdef CONFIG_PLAT_KERNELDUMP
	/*set tfa flush cache size*/
	base = memblock_start_of_DRAM();
	os_size = memblock_end_of_DRAM() - base;
	if (dst_sec_call(DST_SET_OS_MEM_SIZE, base, os_size, 0))
		DST_ERR("set os memory failed.\n");
#endif

#ifdef CONFIG_PLAT_SDEI_EXCEPTIONS_TEST
	sdei_debug_init();
#endif

	DST_PR_END();

	return 0;
}

late_initcall(dst_tfa_trace_init);

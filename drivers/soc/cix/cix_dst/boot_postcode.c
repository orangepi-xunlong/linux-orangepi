// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#include <linux/soc/cix/boot_postcode.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mntn_public_interface.h>

#define bootup_postcode_unused(x) (void)(x)
#define MAX_POSTCODE_SIZE 20
#define SHIFT_BITS 8


static u32 g_postcode_value = 0;
static u64 g_bootup_keypoint_addr = 0;

void set_bootup_postcode(u32 value)
{
	if (value < STAGE_KERNEL_START || value > STAGE_END) {
		pr_err("value[%u] is invalid\n", value);
		return;
	}

	if (!g_bootup_keypoint_addr)
		return;

	pr_debug("%s: set boot keypoint=0x%x", __func__, value);
	g_postcode_value = value;
	writel(value << SHIFT_BITS, (void *)(uintptr_t)g_bootup_keypoint_addr);
}

u32 get_bootup_postcode(void)
{
	return g_postcode_value;
}

static int bootup_postcode_show(struct seq_file *m, void *v)
{
	bootup_postcode_unused(v);

	seq_printf(m, "postcode=0x%x \n", g_postcode_value);
	return 0;
}

static int bootup_postcode_open(struct inode *inode, struct file *file)
{
	return single_open(file, bootup_postcode_show, inode->i_private);
}

static ssize_t bootup_postcode_write(struct file *filp, const char *ubuf,
	size_t cnt, loff_t *data)
{
	char buf[MAX_POSTCODE_SIZE] = {0};
	long value;
	int ret;

	bootup_postcode_unused(filp);
	bootup_postcode_unused(data);

	if (cnt >= MAX_POSTCODE_SIZE) {
		return -EINVAL;	
	}

	if (copy_from_user(&buf, ubuf, cnt))
		return -EINVAL;

	buf[cnt] = 0;

	ret = kstrtol(buf, 0, (long*)(&value));
	if (ret) {
		pr_err("write postcode parameter error \n");
		return ret;
	}

	if (value <= STAGE_KERNEL_BOOTANIM_COMPLETE || value > STAGE_END) {
		pr_err("%s: value[%u] is invalid\n", __func__, (u32)value);
		return -EINVAL;
	}

	pr_info("%s: set postcode 0x%x \n", __func__, (u32)value);
	set_bootup_postcode((u32)value);

	return cnt;
}

static const struct proc_ops bootup_postcode_fops = {
	.proc_open = bootup_postcode_open,
	.proc_write = bootup_postcode_write,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __init init_bootup_postcode(void)
{
	struct proc_dir_entry *pe = NULL;

	pe = proc_create("postcode", 0664, NULL, &bootup_postcode_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}

/*
 * Description:    init bootup_keypoint_addr
 * Input:          NA
 * Output:         NA
 * Return:         NA
 */
static void bootup_keypoint_addr_init(void)
{
	g_bootup_keypoint_addr =
		(uintptr_t)ioremap_wc(POST_CODE_ADDR, sizeof(int));

	pr_info("post code addr: 0x%x, 0x%llx \n", POST_CODE_ADDR,
		g_bootup_keypoint_addr);
}

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init early_stage_init(void)
{
	bootup_keypoint_addr_init();
	set_bootup_postcode(STAGE_KERNEL_EARLY_INITCALL);
	return 0;
}
early_initcall(early_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init pure_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_PURE_INITCALL);
	return 0;
}
pure_initcall(pure_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init core_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_CORE_INITCALL);
	return 0;
}
core_initcall(core_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init core_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_CORE_INITCALL_SYNC);
	return 0;
}
core_initcall_sync(core_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init postcore_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_POSTCORE_INITCALL);
	return 0;
}
postcore_initcall(postcore_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init postcore_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_POSTCORE_INITCALL_SYNC);
	return 0;
}
postcore_initcall_sync(postcore_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init arch_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_ARCH_INITCALL);
	return 0;
}
arch_initcall(arch_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init arch_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_ARCH_INITCALLC);
	return 0;
}
arch_initcall_sync(arch_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init subsys_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_SUBSYS_INITCALL);
	return 0;
}
subsys_initcall(subsys_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init subsys_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_SUBSYS_INITCALL_SYNC);
	return 0;
}
subsys_initcall_sync(subsys_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init fs_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_FS_INITCALL);
	return 0;
}
fs_initcall(fs_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init fs_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_FS_INITCALL_SYNC);
	return 0;
}
fs_initcall_sync(fs_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init rootfs_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_ROOTFS_INITCALL);
	return 0;
}
rootfs_initcall(rootfs_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init device_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_DEVICE_INITCALL);
	return 0;
}
device_initcall(device_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init device_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_DEVICE_INITCALL_SYNC);
	return 0;
}
device_initcall_sync(device_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init late_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_LATE_INITCALL);
	return 0;
}
late_initcall(late_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init late_sync_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_LATE_INITCALL_SYNC);
	init_bootup_postcode();
	return 0;
}
late_initcall_sync(late_sync_stage_init);

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init console_stage_init(void)
{
	set_bootup_postcode(STAGE_KERNEL_CONSOLE_INITCALL);
	return 0;
}
console_initcall(console_stage_init);

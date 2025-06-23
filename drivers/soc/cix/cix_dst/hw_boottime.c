/*
 * drivers/boottime/hw_boottime.c
 *
 * boottime check
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include <linux/soc/cix/hw_boottime.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/io.h>
#include <mntn_public_interface.h>

#define BOOT_LOG_NUM 64
#define TRAS_MS 10
#define BOOT_TIME_CHECK 1000000000
#define boot_time_unused(x) (void)(x)

struct boot_log_struct {
	uint64_t time_us;
	char event[BOOT_STR_SIZE];
} hw_boottime[BOOT_LOG_NUM];

static unsigned int g_drv_boottime = 50000;
static uint32_t *psys_cont_addr;

int boot_log_count;
static DEFINE_MUTEX(hw_boottime_lock);
static int hw_boottime_enabled = 1;
#define BOOTUP_DONE "[INFOR]_wm_boot_animation_done"

static int __init early_parse_drv_bootime_cmdline(char *boot_time_cmdline)
{
	int tmp;

	if (get_option(&boot_time_cmdline, &tmp)) {
		pr_info("%s: drv_bootime=%d", __func__, tmp);
		g_drv_boottime = tmp;
		return 0;
	}

	return -EINVAL;
}

early_param("drv_bootime", early_parse_drv_bootime_cmdline);

static uint64_t sky1_get_time_us(void)
{
	uint64_t old_upper, upper;
	uint32_t lower;

	if (!psys_cont_addr) {
		psys_cont_addr = (uint32_t *)ioremap_wc(CIX_SYS_COUNTER_BASE, 0x100);
		if (!psys_cont_addr) {
			pr_err("sky1_get_time_ms: ioremap failed\n");
			return 0;
		}
	}

	upper = *(psys_cont_addr + 1);
	do {
		old_upper = upper;
		lower = *psys_cont_addr;
		upper = *(psys_cont_addr + 1);
	} while (upper != old_upper);

	upper <<= 32;
	upper |= lower;
	return upper / 100;
}

int __init_or_module do_boottime_initcall(initcall_t fn)
{
	int ret;
	uint64_t duration;
	uint64_t calltime;
	char log_info[BOOT_STR_SIZE] = {0};
	int sec;

	calltime = sky1_get_time_us();
	ret = fn();
	duration = sky1_get_time_us() - calltime;

	if (duration > g_drv_boottime) {
		pr_info("%s, initcall %pS returned %d after %lld usecs\n",
			__func__, fn, ret, duration);
		sec = snprintf(log_info, sizeof(log_info) - 1,
			"[WARNING] %-10lld usecs | %pS", duration, fn);
		if (sec == -1)
			printk("do_boottime_initcall snprintf_s error\n");
		boot_record(log_info);
	}
	return ret;
}

void boot_record(const char *str)
{
	if (hw_boottime_enabled == 0)
		return;

	if (boot_log_count >= BOOT_LOG_NUM) {
		printk("[boottime] no enough boottime buffer\n");
		return;
	}
	if (strncmp(BOOTUP_DONE, str, strlen(BOOTUP_DONE)) == 0)
		hw_boottime_enabled = 0; // zygote start
	mutex_lock(&hw_boottime_lock);
	hw_boottime[boot_log_count].time_us = sky1_get_time_us();
	memset(&hw_boottime[boot_log_count].event,
		0, sizeof(hw_boottime[boot_log_count].event));
	strncpy((char *)&hw_boottime[boot_log_count].event, str, strlen(str));

	boot_log_count++;
	mutex_unlock(&hw_boottime_lock);
}
EXPORT_SYMBOL(boot_record);

static int hw_boottime_show(struct seq_file *m, void *v)
{
	int i;
	uint64_t boot_time = 0;
	struct fw_boot_phase_point *fwp;

	boot_time_unused(v);
	/* printf times */
	seq_puts(m, "----------- BOOT TIME (FW & Bootloader) -----------\n");
	fwp = (struct fw_boot_phase_point *)ioremap_wc(FW_BOOT_PERF_BASE,
		sizeof(struct fw_boot_phase_point) * FW_BOOT_PHASE_MAX);
	if (fwp) {
		for (i = 0; i < FW_BOOT_PHASE_MAX; i++) {
			boot_time = fwp[i].end - fwp[i].start;
			if (boot_time != 0) {
				seq_printf(m, "Firmware-%s boot %lldms\n",
					fwp[i].fw_name, boot_time);
			}
		}
	}

	seq_puts(m, "\n----------- BOOT TIME (kernel) -----------\n");
	seq_printf(m, "%-15s|%-28s| func\n", " start time", "time");
	for (i = 0; i < boot_log_count; i++) {
		seq_printf(m,
			"---------------+----------------------------+----------------------------\n");
		seq_printf(m, "%lld us (%lld ms) | %s \n", hw_boottime[i].time_us,
			(hw_boottime[i].time_us - hw_boottime[0].time_us) / 1000, hw_boottime[i].event);
	}
	seq_printf(m, "\n   %s",
		hw_boottime_enabled ? "starting..." : "start done");
	return 0;
}

static int hw_boottime_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_boottime_show, inode->i_private);
}

static ssize_t hw_boottime_write(struct file *filp, const char *ubuf,
	size_t cnt, loff_t *data)
{
	char buf[BOOT_STR_SIZE] = {0};
	size_t copy_size = cnt;

	boot_time_unused(filp);
	boot_time_unused(data);
	if (cnt >= sizeof(buf))
		copy_size = BOOT_STR_SIZE - 1;
	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	buf[copy_size] = 0;
	boot_record(buf);
	return cnt;
}

static const struct proc_ops hw_boottime_ops = {
	.proc_open = hw_boottime_open,
	.proc_write = hw_boottime_write,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __init init_boot_time(void)
{
	struct proc_dir_entry *pe = NULL;

	pe = proc_create("boottime", 0664, NULL, &hw_boottime_ops);
	if (!pe)
		return -ENOMEM;
	return 0;
}
module_init(init_boot_time);

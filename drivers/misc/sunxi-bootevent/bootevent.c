/*
 * Copyright(c) 2017-2018 Allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>

#define BOOT_STR_SIZE 256
#define BUF_COUNT 12
#define LOGS_PER_BUF 80
#define TRACK_TASK_COMM
#define MSG_SIZE 128

#define SEQ_printf(m, x...)	    \
	do {			    \
		if (m)		    \
			seq_printf(m, x);	\
		else		    \
			pr_err(x);	    \
	} while (0)


struct log_t {
	char *comm_event;
#ifdef TRACK_TASK_COMM
	pid_t pid;
#endif
	u64 timestamp;
};

static struct log_t *bootevent[BUF_COUNT];
static unsigned long log_count;
static bool bootevent_enabled;
static u64 timestamp_on, timestamp_off;

static DEFINE_MUTEX(bootevent_lock);

long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

void log_boot(char *str)
{
	unsigned long long ts;
	struct log_t *p = NULL;
	size_t n = strlen(str) + 1;

	if (!bootevent_enabled)
		return;
	ts = sched_clock();

	mutex_lock(&bootevent_lock);
	if (log_count >= (LOGS_PER_BUF * BUF_COUNT)) {
		pr_err("[BOOTEVENT] not enuough bootevent buffer\n");
		goto out;
	} else if (log_count && !(log_count % LOGS_PER_BUF)) {
		bootevent[log_count / LOGS_PER_BUF] =
			kzalloc(sizeof(struct log_t) * LOGS_PER_BUF,
				GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN);
	}
	if (!bootevent[log_count / LOGS_PER_BUF]) {
		pr_err("no memory for bootevent\n");
		goto out;
	}
	p = &bootevent[log_count / LOGS_PER_BUF][log_count % LOGS_PER_BUF];

	p->timestamp = ts;
#ifdef TRACK_TASK_COMM
	p->pid = current->pid;
	n += TASK_COMM_LEN;
#endif
	p->comm_event = kzalloc(n, GFP_ATOMIC | __GFP_NORETRY |
			  __GFP_NOWARN);
	if (!p->comm_event) {
		bootevent_enabled = false;
		goto out;
	}
#ifdef TRACK_TASK_COMM
	memcpy(p->comm_event, current->comm, TASK_COMM_LEN);
	memcpy(p->comm_event + TASK_COMM_LEN, str, n - TASK_COMM_LEN);
#else
	memcpy(p->comm_event, str, n);
#endif
	log_count++;
out:
	mutex_unlock(&bootevent_lock);
}


void bootevent_initcall(initcall_t fn, unsigned long long ts)
{
#define INITCALL_THRESHOLD 15000000
	/* log more than 15ms initcalls */
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];

	if (ts > INITCALL_THRESHOLD) {
		msec_rem = do_div(ts, NSEC_PER_MSEC);
		snprintf(msgbuf, MSG_SIZE, "initcall: %pf %5llu.%06lums",
			 fn, ts, msec_rem);
		log_boot(msgbuf);
	}
}

void bootevent_probe(unsigned long long ts, struct device *dev,
			   struct device_driver *drv, unsigned long probe)
{
#define PROBE_THRESHOLD 15000000
	/* log more than 15ms probes*/
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];
	int pos = 0;

	if (ts <= PROBE_THRESHOLD)
		return;
	msec_rem = do_div(ts, NSEC_PER_MSEC);

	pos += snprintf(msgbuf, MSG_SIZE, "probe: probe=%pf", (void *)probe);
	if (drv)
		pos += snprintf(msgbuf + pos, MSG_SIZE - pos, " drv=%s(%p)",
				drv->name ? drv->name : "",
				(void *)drv);

	if (dev && dev->init_name)
		pos += snprintf(msgbuf + pos, MSG_SIZE - pos, " dev=%s(%p)",
				dev->init_name, (void *)dev);

	pos += snprintf(msgbuf + pos, MSG_SIZE - pos, " %5llu.%06lums",
			ts, msec_rem);

	log_boot(msgbuf);
}

void
bootevent_pdev_register(unsigned long long ts, struct platform_device *pdev)
{
#define PROBE_THRESHOLD 15000000
	/* log more than 15ms probes*/
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];

	if (ts <= PROBE_THRESHOLD || !pdev)
		return;
	msec_rem = do_div(ts, NSEC_PER_MSEC);
	snprintf(msgbuf, MSG_SIZE, "probe: pdev=%s(%p) %5llu.%06lums",
		 pdev->name, (void *)pdev, ts, msec_rem);

	log_boot(msgbuf);
}

static void sunxi_bootevent_switch(int on)
{
	mutex_lock(&bootevent_lock);
	if (bootevent_enabled ^ on) {
		unsigned long long ts = sched_clock();

		pr_err("BOOTEVENT:%10Ld.%06ld: %s\n",
		       nsec_high(ts), nsec_low(ts), on ? "ON" : "OFF");

		if (on) {
			bootevent_enabled = 1;
			timestamp_on = ts;
		} else {
			/* boot up complete */
			bootevent_enabled = 0;
			timestamp_off = ts;
		}
	}
	mutex_unlock(&bootevent_lock);
}

static ssize_t
sunxi_bootevent_write(struct file *filp, const char *ubuf,
						size_t cnt, loff_t *data)
{
	char buf[BOOT_STR_SIZE];
	size_t copy_size = cnt;

	if (cnt >= sizeof(buf))
		copy_size = BOOT_STR_SIZE - 1;

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	if (cnt == 1 && buf[0] == '1') {
		sunxi_bootevent_switch(1);
		return 1;
	} else if (cnt == 1 && buf[0] == '0') {
		sunxi_bootevent_switch(0);
		return 1;
	}

	buf[copy_size] = 0;
	log_boot(buf);

	return cnt;

}

static int sunxi_bootevent_show(struct seq_file *m, void *v)
{
	int i;
	struct log_t *p;

	SEQ_printf(m, "----------------------------------------\n");
	SEQ_printf(m, "%d BOOTEVENT (unit:msec)\n", bootevent_enabled);
	SEQ_printf(m, "----------------------------------------\n");
	SEQ_printf(m, "%10Ld.%06ld : ON\n",
		   nsec_high(timestamp_on), nsec_low(timestamp_on));

	for (i = 0; i < log_count; i++) {
		p = &bootevent[i / LOGS_PER_BUF][i % LOGS_PER_BUF];
		if (!p->comm_event)
			continue;
#ifdef TRACK_TASK_COMM
#define FMT "%10Ld.%06ld :%5d-%-16s: %s\n"
#else
#define FMT "%10Ld.%06ld : %s\n"
#endif
		SEQ_printf(m, FMT, nsec_high(p->timestamp),
			   nsec_low(p->timestamp),
#ifdef TRACK_TASK_COMM
			   p->pid, p->comm_event, p->comm_event + TASK_COMM_LEN
#else
			   p->comm_event
#endif
			   );
	}

	SEQ_printf(m, "%10Ld.%06ld : OFF\n",
		   nsec_high(timestamp_off), nsec_low(timestamp_off));
	SEQ_printf(m, "----------------------------------------\n");
	return 0;
}

static int sunxi_bootevent_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_bootevent_show, inode->i_private);
}

static const struct file_operations sunxi_bootevent_fops = {
	.open    = sunxi_bootevent_open,
	.write   = sunxi_bootevent_write,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int __init init_boot_event(void)
{
	struct proc_dir_entry *pe;

	memset(bootevent, 0, sizeof(struct log_t *) * BUF_COUNT);
	bootevent[0] = kzalloc(sizeof(struct log_t) * LOGS_PER_BUF,
			      GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN);
	if (!bootevent[0])
		goto fail;
	sunxi_bootevent_switch(1);

	pe = proc_create("bootevent", 0664, NULL, &sunxi_bootevent_fops);
	if (!pe)
		return -ENOMEM;

fail:
	return 0;
}

early_initcall(init_boot_event);

MODULE_AUTHOR("Allwinnertech.com");
MODULE_DESCRIPTION("bootevent driver for debug bootting time");
MODULE_VERSION("1.3.0");
MODULE_LICENSE("GPL");

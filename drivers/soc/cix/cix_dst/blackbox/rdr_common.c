// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_common.c
 *
 * blackbox common functions moudle
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

#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/acpi.h>
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_subtype_exception.h>
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"
#include "rdr_print.h"

#define HIMNTN_FIRST_NV_DATA 0
#define HIMNTN_SECOND_NV_DATA 1
#define HIMNTN_THIRD_NV_DATA 2
#define HIMNTN_FORTH_NV_DATA 3
#define HIMNTN_FIFTH_NV_DATA 4
#define HIMNTN_SIXTH_NV_DATA 5
#define RDR_DUMPCTRL_LENGTH 16
#define TIMELEN 8
#define DATELEN 11

typedef struct rdr_pdev {
	struct platform_device *plat_device;
	bbox_mem ramlog;
	bbox_mem rdr;
	RDR_NVE nve;
	uint32_t max_logsize;
	struct rdr_area_data area_data;
} rdr_pdev;

enum RDR_DTS_DATA_INDX {
	MAX_LOGNUM = 0,
	DUMPLOG_TIMEOUT,
	REBOOT_TIMES,
	RDR_DTS_U32_NUMS,
};

/* Kernel native interface in version.c, but not declared in version.h */
extern int get_kernel_build_time(char *blddt, int dtlen, char *bldtm,
				 int tmlen);

#undef RDR_CORE_DEF
#define RDR_CORE_DEF(name, index) MNTN_DEF_ARGS([index] = #name)
char *rdr_core_name[] = { RDR_CORE_LIST };

atomic_t bb_in_suspend = ATOMIC_INIT(0);
atomic_t bb_in_reboot = ATOMIC_INIT(0);
atomic_t bb_in_saving = ATOMIC_INIT(0);

#undef MODID_START_END_DEF
#define MODID_START_END_DEF(name, start, end) \
	{ .modid_start = (start), .modid_end = (end), .modid_str = #name }
struct blackbox_modid_list g_modid_list[] = {
	MODID_START_END_LIST,
};
static rdr_pdev g_rdr_device = { 0 };
struct rdr_dts_prop {
	int indx;
	const char *propname;
	u32 data;
} g_rdr_dts_data[RDR_DTS_U32_NUMS] = {
	{ MAX_LOGNUM, "rdr-log-max-nums", 0 },
	{ DUMPLOG_TIMEOUT, "wait-dumplog-timeout", 0 },
	{ REBOOT_TIMES, "unexpected-max-reboot-times", 0 },
};

void rdr_get_builddatetime(u8 *out, u32 out_len)
{
	u8 *pout = out;
	u8 *p = NULL;
	u8 date[DATELEN + 1] = { 0 };
	u8 time[TIMELEN + 1] = { 0 };
	int cnt = RDR_BUILD_DATE_TIME_LEN, ret = 0;

	if (out == NULL) {
		BB_ERR("out is null!\n");
		return;
	}

	if (out_len < RDR_BUILD_DATE_TIME_LEN) {
		BB_ERR("out_len is too small!\n");
		return;
	}

	memset((void *)out, 0, out_len);

	ret = get_kernel_build_time(date, DATELEN, time, TIMELEN);
	if (ret) {
		BB_ERR("get kernel build time failed!\n");
		goto error;
	}
	date[DATELEN] = '\0';
	time[TIMELEN] = '\0';

	p = date;
	while (*p) {
		if (!cnt)
			goto error;
		if (*p != ' ') {
			*pout++ = *p++;
			cnt--;
		} else {
			p++;
		}
	}

	p = time;
	while (*p) {
		if (!cnt)
			goto error;
		if (*p != ':') {
			*pout++ = *p++;
			cnt--;
		} else {
			p++;
		}
	}

error:
	out[RDR_BUILD_DATE_TIME_LEN - 1] = '\0';
}

u64 rdr_get_tick(void)
{
	/* use only one int value to save time: */
	struct timespec64 uptime;

	ktime_get_boottime_ts64(&uptime);
	return (u64)uptime.tv_nsec;
}

char *rdr_get_timestamp(void)
{
	struct rtc_time tm;
	static char databuf[DATA_MAXLEN + 1];
	struct timespec64 tv;

	BB_PR_START();
	memset(databuf, 0, DATA_MAXLEN + 1);
	ktime_get_real_ts64(&tv);
	rtc_time64_to_tm(tv.tv_sec, &tm);

	(void)snprintf(databuf, DATA_MAXLEN + 1, "%04d%02d%02d%02d%02d%02d",
		       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		       tm.tm_min, tm.tm_sec);

	BB_DBG("[%s] !\n", databuf);
	BB_PR_END();
	return databuf;
}

int rdr_get_suspend_state(void)
{
	return atomic_read(&bb_in_suspend);
}

int rdr_get_reboot_state(void)
{
	return atomic_read(&bb_in_reboot);
}

void rdr_set_saving_state(int state)
{
	return atomic_set(&bb_in_saving, state);
}

#ifdef CONFIG_PM
static struct notifier_block bb_suspend_notifier;
static int bb_suspend_nb(struct notifier_block *this, unsigned long event,
			 void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		BB_DBG("resume +\n");
		atomic_set(&bb_in_suspend, 0);
		BB_DBG("resume -\n");
		break;

	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		BB_DBG("suspend +\n");
		atomic_set(&bb_in_suspend, 1);
		while (1) {
			if (atomic_read(&bb_in_saving))
				msleep(1000);
			else
				break;
		}
		BB_DBG("suspend -\n");
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}
#endif

static int bb_reboot_nb(struct notifier_block *nb, unsigned long foo, void *bar)
{
	int i = 10;
	/* prevent access the emmc now: */
	BB_PN("shutdown +\n");
	atomic_set(&bb_in_reboot, 1);
	while (i--) {
		if (atomic_read(&bb_in_saving))
			msleep(1000);
		else
			break;
		BB_DBG("rdr:is saving rdr, wait 1s\n");
	}
	rdr_field_reboot_done();
	BB_PN("shutdown -\n");

	return 0;
}

u32 rdr_get_reboot_times(void)
{
	return g_rdr_dts_data[REBOOT_TIMES].data;
}

bbox_mem rdr_ramlog_mem(void)
{
	return g_rdr_device.ramlog;
}

bbox_mem rdr_reserved_mem(void)
{
	return g_rdr_device.rdr;
}

struct rdr_area_data rdr_get_area_data(void)
{
	return g_rdr_device.area_data;
}

u64 rdr_get_logsize(void)
{
	return g_rdr_device.max_logsize;
}

RDR_NVE rdr_get_nve(void)
{
	return g_rdr_device.nve;
}

u32 rdr_get_lognum(void)
{
	return g_rdr_dts_data[MAX_LOGNUM].data;
}

char *blackbox_get_modid_str(u32 modid)
{
	u32 i;
	u32 modid_size = ARRAY_SIZE(g_modid_list);

	for (i = 0; i < modid_size; ++i) {
		if (modid >= g_modid_list[i].modid_start &&
		    modid <= g_modid_list[i].modid_end)
			return g_modid_list[i].modid_str;
	}

	return "UNDEF";
}

static int rdr_get_property_data_u32(struct device *dev)
{
	u32 value = 0;
	int i, ret;

	if (dev == NULL) {
		BB_ERR("parameter device dev is NULL!\n");
		return -1;
	}

	for (i = 0; i < RDR_DTS_U32_NUMS; i++) {
		ret = device_property_read_u32(dev, g_rdr_dts_data[i].propname,
					       &value);
		if (ret) {
			BB_ERR("cannot find g_rdr_dts_data[%d],[%s] in dts!\n",
			       i, g_rdr_dts_data[i].propname);
			return ret;
		}

		g_rdr_dts_data[i].data = value;
		BB_DBG("get %s [0x%x] in dts!\n", g_rdr_dts_data[i].propname,
		       value);
	}

	return 0;
}

static void *bbox_vmap(phys_addr_t paddr, size_t size)
{
	int i;
	void *vaddr = NULL;
	unsigned long offset;
	int pages_count;
	struct page **pages = NULL;

	offset = paddr & ~PAGE_MASK;
	paddr &= PAGE_MASK;
	pages_count = PAGE_ALIGN(size + offset) / PAGE_SIZE;

	pages = kcalloc(pages_count, sizeof(struct page *), GFP_KERNEL);
	if (pages == NULL)
		return NULL;

	for (i = 0; i < pages_count; i++)
		*(pages + i) = phys_to_page((uintptr_t)(paddr + PAGE_SIZE * i));

	vaddr = vmap(pages, pages_count, VM_MAP, PAGE_KERNEL);
	kfree(pages);
	if (vaddr == NULL)
		return NULL;

	return offset + (char *)vaddr;
}

int rdr_common_early_init(struct platform_device *pdev)
{
	int i, ret, len;
	struct device *dev = NULL;
	const char *prdr_dumpctrl = NULL;
	struct rdr_area_data *rdr_area_data = &g_rdr_device.area_data;
	struct reserved_mem *rmem;
	struct device_node *np;
	struct resource *res;

	rdr_area_data->value = 0;
	dev = &pdev->dev;
	if (device_property_read_u64(dev, "ramlog_addr",
				     &g_rdr_device.ramlog.paddr)) {
		BB_DBG("get ramlog address from dts failed...\n");
		g_rdr_device.ramlog.paddr = 0;
	}

	if (device_property_read_u64(dev, "ramlog_size",
				     &g_rdr_device.ramlog.size)) {
		BB_DBG("get ramlog size from dts failed...\n");
		g_rdr_device.ramlog.size = 0;
	}

	if (g_rdr_device.ramlog.paddr && g_rdr_device.ramlog.size) {
		g_rdr_device.ramlog.vaddr = bbox_vmap(g_rdr_device.ramlog.paddr,
						      g_rdr_device.ramlog.size);
		BB_DBG("ramlog address=0x%px\n", g_rdr_device.ramlog.vaddr);
	}

	if (!has_acpi_companion(dev)) {
		np = of_parse_phandle(dev->of_node, "memory-region", 0);
		if (!np)
			return -EINVAL;
		rmem = of_reserved_mem_lookup(np);
		if (!rmem) {
			BB_ERR("No reserved memory assigned to the device\n");
			return -ENODEV;
		}
		g_rdr_device.rdr.paddr = rmem->base;
		g_rdr_device.rdr.size = rmem->size;
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res)
			return -EINVAL;
		g_rdr_device.rdr.paddr = res->start;
		g_rdr_device.rdr.size = resource_size(res);
	}

	BB_DBG("get phymem addr [%llx] size [%llx]in dts/acpi!\n",
	       g_rdr_device.rdr.paddr, g_rdr_device.rdr.size);

	if (g_rdr_device.rdr.paddr && g_rdr_device.rdr.size) {
		g_rdr_device.rdr.vaddr = bbox_vmap(g_rdr_device.rdr.paddr,
						   g_rdr_device.rdr.size);
		BB_DBG("rdr address=0x%px\n", g_rdr_device.rdr.vaddr);
	}

	ret = device_property_read_string(dev, "rdr-dumpctl", &prdr_dumpctrl);
	if (ret < 0 || prdr_dumpctrl == NULL ||
	    strlen(prdr_dumpctrl) > RDR_DUMPCTRL_LENGTH) {
		BB_ERR("find rdr-dumpctl node fail! [%s]\n", prdr_dumpctrl);
		return ret;
	}
	BB_DBG("get prdr_dumpctrl [%s] in dts!\n", prdr_dumpctrl);
	g_rdr_device.nve = 0;
	len = (int)strlen(prdr_dumpctrl);
	for (i = --len; i >= 0; i--) {
		if (prdr_dumpctrl[i] == '1')
			g_rdr_device.nve |= (u64)1 << (unsigned int)(len - i);
	}
	BB_DBG("get nve [0x%llx] in dts!\n", g_rdr_device.nve);

	ret = device_property_read_u32(dev, "rdr-log-max-size",
				       &g_rdr_device.max_logsize);
	if (ret) {
		BB_ERR("cannot find rdr-log-max-size in dts!\n");
		return ret;
	}
	BB_DBG("get rdr-log-max-size [0x%x] in dts!\n",
	       g_rdr_device.max_logsize);

	ret = rdr_get_property_data_u32(&pdev->dev);
	if (ret < 0)
		return ret;

	ret = device_property_read_u32(dev, "rdr_area_num",
				       &rdr_area_data->value);
	if (ret) {
		BB_ERR("cannot find rdr_area_num in dts!\n");
		return ret;
	}
	BB_DBG("get rdr_area_num [0x%x] in dts!\n", rdr_area_data->value);

	if (rdr_area_data->value > RDR_CORE_MAX) {
		BB_ERR("invaild core num in dts!\n");
		return -1;
	}
	ret = device_property_read_u32_array(dev, "rdr_area_sizes",
					     &rdr_area_data->data[0],
					     rdr_area_data->value);
	if (ret) {
		BB_ERR("cannot find rdr_area_sizes in dts!\n");
		return ret;
	}

	ret = rdr_field_init(rdr_area_data);
	if (ret != 0)
		return -ENOMEM;

	return ret;
}

static struct notifier_block bb_reboot_notifier;
int rdr_common_init(void)
{
#ifdef CONFIG_PM
	/* Register to get PM events */
	bb_suspend_notifier.notifier_call = bb_suspend_nb;
	bb_suspend_notifier.priority = -1;
	if (register_pm_notifier(&bb_suspend_notifier)) {
		BB_ERR("Failed to register for PM events\n");
		return -1;
	}
#endif

	bb_reboot_notifier.notifier_call = bb_reboot_nb;
	bb_reboot_notifier.priority = -1;
	if (register_reboot_notifier(&bb_reboot_notifier)) {
		BB_ERR("Failed to register for Reboot events\n");
		return -1;
	}
	return 0;
}

void *rdr_bbox_map(phys_addr_t paddr, size_t size)
{
	void *vaddr = NULL;

	if (paddr < g_rdr_device.rdr.paddr || !size ||
	    ((paddr + size) < paddr) ||
	    (paddr + size) > (g_rdr_device.rdr.paddr + g_rdr_device.rdr.size)) {
		BB_ERR("Error BBox memory\n");
		return NULL;
	}

	if (pfn_valid(g_rdr_device.rdr.paddr >> PAGE_SHIFT))
		vaddr = g_rdr_device.rdr.vaddr +
			(paddr - g_rdr_device.rdr.paddr);
	else
		vaddr = ioremap_wc(paddr, size);

	return vaddr;
}
EXPORT_SYMBOL(rdr_bbox_map);

void rdr_bbox_unmap(const void *vaddr)
{
	if (vaddr == NULL)
		return;
	if (!pfn_valid(g_rdr_device.rdr.paddr >> PAGE_SHIFT) &&
	    vaddr == g_rdr_device.rdr.vaddr)
		iounmap((void __iomem *)vaddr);
}
EXPORT_SYMBOL(rdr_bbox_unmap);

void rdr_flush_total_mem(void)
{
	dcache_clean_poc((u64)g_rdr_device.rdr.vaddr,
			 (u64)g_rdr_device.rdr.vaddr + g_rdr_device.rdr.size);
}

/*
 * Description:  After the log directory corresponding to each exception is saved,
 *               this function needs to be called to indicate that the directory has been recorded
 *               and it can be packaged and uploaded by logserver.
 * Input:        logpath: the directory where the log is saved corresponding to the exception;
 *               step:the step which the exception log is saved in, and whether to continue using the flag;
 * Other:        used by rdr_core.c, rdr_hisi_ap_adapter.c
 */
void bbox_save_done(const char *logpath, u32 step)
{
	struct file *file;
	int ret;
	char path[PATH_MAXLEN];
	u32 len;

	BB_PR_START();

	if (logpath == NULL ||
	    (strlen(logpath) + strlen(BBOX_SAVE_DONE_FILENAME) + 1) >
		    PATH_MAXLEN) {
		BB_ERR("logpath is invalid\n");
		return;
	}

	BB_PN("logpath is [%s], step is [%u]\n", logpath, step);
	if (step == BBOX_SAVE_STEP_DONE) {
		/* combine the absolute path of the done file as a parameter of sys_mkdir */
		memset(path, 0, PATH_MAXLEN);
		len = strlen(logpath);
		if (len >= PATH_MAXLEN) {
			BB_ERR("memcpy err\n]");
			return;
		}
		memcpy(path, logpath, len);

		strncat(path, BBOX_SAVE_DONE_FILENAME,
			((PATH_MAXLEN - 1) - strlen(path)));
		if (strncmp(path, PATH_ROOT, strlen(PATH_ROOT)) != 0) {
			BB_ERR("path [%s] err\n]", path);
			return;
		}

		/* create a done file under the timestamp directory */
		file = filp_open(path, O_CREAT | O_WRONLY, FILE_LIMIT);
		if (IS_ERR(file)) {
			BB_ERR("create [%s] error\n", path);
			return;
		}
		filp_close(file, NULL);

		/*
		 * according to the permission requirements,
		 * the hisi_logs directory and subdirectory group are adjusted to root-system
		 */
		ret = (int)rdr_chown((const char __user *)path, ROOT_UID,
				     SYSTEM_GID, false);
		if (ret)
			BB_ERR("chown %s uid [%d] gid [%d] failed err [%d]!\n",
			       PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
	}

	BB_PR_END();
}

/*
 * Description:  save reboot times to specified memory
 */
void rdr_record_reboot_times2mem(void)
{
	struct rdr_struct_s *pbb = NULL;

	BB_PR_START();
	pbb = rdr_get_head(false);
	pbb->base_info.reserve = RDR_UNEXPECTED_REBOOT_MARK_ADDR;
	BB_PR_END();
}

/*
 * Description:   reset the file saving reboot times
 */
void rdr_reset_reboot_times(void)
{
	struct file *fp = NULL;
	ssize_t length;
	char buf;

	BB_PR_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, FILE_LIMIT);
	if (IS_ERR(fp)) {
		BB_ERR("open %s fail\n", RDR_REBOOT_TIMES_FILE);
		return;
	}
	buf = 0;
	vfs_llseek(fp, 0L, SEEK_SET);
	length = kernel_write(fp, &buf, sizeof(buf), &(fp->f_pos));
	if (length == sizeof(buf))
		vfs_fsync(fp, 0);

	filp_close(fp, NULL);
	BB_PR_END();
}

/*
 * Description:   record the reboot times to file.
 * Return:        int: reboot times.
 */
int rdr_record_reboot_times2file(void)
{
	struct file *fp = NULL;
	ssize_t length;
	char buf = 0;

	BB_PR_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, FILE_LIMIT);
	if (IS_ERR(fp)) {
		BB_ERR("open %s fail\n", RDR_REBOOT_TIMES_FILE);
		return 0;
	}

	vfs_llseek(fp, 0L, SEEK_SET);
	length = kernel_read(fp, &buf, sizeof(buf), &fp->f_pos);
	if (length == 0 || buf == 0)
		buf = 0;
	buf++;

	vfs_llseek(fp, 0L, SEEK_SET);
	length = kernel_write(fp, &buf, sizeof(buf), &(fp->f_pos));
	if (length == sizeof(buf))
		vfs_fsync(fp, 0);

	filp_close(fp, NULL);
	BB_PR_END();
	return buf;
}

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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time64.h>
#include <linux/uaccess.h>
#include <linux/rtc.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/atomic.h>
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
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_subtype_exception.h>
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"
#include "rdr_print.h"
#include "rdr_utils.h"

#undef __REBOOT_REASON_MAP
#define __REBOOT_REASON_MAP(x, y) { #x, x, #y, y },
struct cmdword reboot_reason_map[] = {
	#include <mntn_reboot_reason_map.h>
};
#undef __REBOOT_REASON_MAP

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
	uint32_t max_logseze;
	struct rdr_area_data area_data;
} rdr_pdev;

/* Kernel native interface in version.c, but not declared in version.h */
extern int get_kernel_build_time(char *blddt, int dtlen, char *bldtm, int tmlen);

void rdr_get_builddatetime(u8 *out, u32 out_len)
{
	u8 *pout = out;
	u8 *p = NULL;
	u8 date[DATELEN + 1] = {0};
	u8 time[TIMELEN + 1] = {0};
	int cnt = RDR_BUILD_DATE_TIME_LEN, ret = 0;

	if (out == NULL) {
		BB_PRINT_ERR("[%s], out is null!\n", __func__);
		return;
	}

	if (out_len < RDR_BUILD_DATE_TIME_LEN) {
		BB_PRINT_ERR("[%s],out_len is too small!\n", __func__);
		return;
	}

	memset((void *)out, 0, out_len);

	ret = get_kernel_build_time(date, DATELEN, time, TIMELEN);
	if (ret) {
		BB_PRINT_ERR("[%s], get kernel build time failed!\n", __func__);
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

	BB_PRINT_START();
	memset(databuf, 0, DATA_MAXLEN + 1);
	ktime_get_real_ts64(&tv);
	rtc_time64_to_tm(tv.tv_sec, &tm);

	(void)snprintf(databuf, DATA_MAXLEN + 1, "%04d%02d%02d%02d%02d%02d",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);

	BB_PRINT_DBG("rdr: %s [%s] !\n", __func__, databuf);
	BB_PRINT_END();
	return databuf;
}

struct blackbox_modid_list g_modid_list[] = {
	{ PLAT_BB_MOD_MODEM_DRV_START, PLAT_BB_MOD_MODEM_DRV_END, "MODEM DRV" },
	{ PLAT_BB_MOD_MODEM_OSA_START, PLAT_BB_MOD_MODEM_OSA_END, "MODEM OSA" },
	{ PLAT_BB_MOD_MODEM_OM_START, PLAT_BB_MOD_MODEM_OM_END, "MODEM OM" },
	{ PLAT_BB_MOD_MODEM_GU_L2_START, PLAT_BB_MOD_MODEM_GU_L2_END, "MODEM GU L2" },
	{ PLAT_BB_MOD_MODEM_GU_WAS_START, PLAT_BB_MOD_MODEM_GU_WAS_END, " MODEM GU WAS" },
	{ PLAT_BB_MOD_MODEM_GU_GAS_START, PLAT_BB_MOD_MODEM_GU_GAS_END, "MODEM GU GAS" },
	{ PLAT_BB_MOD_MODEM_GU_NAS_START, PLAT_BB_MOD_MODEM_GU_NAS_END, "MODEM GU NAS" },
	{ PLAT_BB_MOD_MODEM_GU_DSP_START, PLAT_BB_MOD_MODEM_GU_DSP_END, "MODEM GU DSP" },
	{ (unsigned int)PLAT_BB_MOD_AP_START, (unsigned int)PLAT_BB_MOD_AP_END, "ap" },
	{ (unsigned int)PLAT_BB_MOD_ATF_START, (unsigned int)PLAT_BB_MOD_ATF_START, "atf" },
	{ (unsigned int)PLAT_BB_MOD_CSUSE_START, (unsigned int)PLAT_BB_MOD_CSUSE_END, "csuse" },
	{ (unsigned int)PLAT_BB_MOD_CSUPM_START, (unsigned int)PLAT_BB_MOD_CSUPM_END, "csupm" },
	{ (unsigned int)PLAT_BB_MOD_UEFI_START, (unsigned int)PLAT_BB_MOD_UEFI_END, "uefi" },
	{ (unsigned int)PLAT_BB_MOD_TEEOS_START, (unsigned int)PLAT_BB_MOD_TEEOS_END, "teeos" },
	{ (unsigned int)PLAT_BB_MOD_SF_START, (unsigned int)PLAT_BB_MOD_SF_END, "sensor fusion" },
	{ (unsigned int)PLAT_BB_MOD_ISP_START, (unsigned int)PLAT_BB_MOD_ISP_END, "isp" },
	{ (unsigned int)PLAT_BB_MOD_HIFI_START, (unsigned int)PLAT_BB_MOD_HIFI_END, "hifi" },
	{ (unsigned int)PLAT_BB_MOD_NPU_START, (unsigned int)PLAT_BB_MOD_NPU_END, "npu" },
	{ (unsigned int)PLAT_BB_MOD_VPU_START, (unsigned int)PLAT_BB_MOD_VPU_END, "vpu" },
	{ (unsigned int)PLAT_BB_MOD_DPU_START, (unsigned int)PLAT_BB_MOD_DPU_END, "dpu" },
	{ (unsigned int)PLAT_BB_MOD_CSI_START, (unsigned int)PLAT_BB_MOD_CSI_END, "csi" },
	{ (unsigned int)PLAT_BB_MOD_CSIDMA_START, (unsigned int)PLAT_BB_MOD_CSIDMA_END, "csidma" },
	{ (unsigned int)PLAT_BB_MOD_NI700_START, (unsigned int)PLAT_BB_MOD_NI700_END, "ni700" },
	{ (unsigned int)PLAT_BB_MOD_GPU_START, (unsigned int)PLAT_BB_MOD_GPU_END, "gpu" },
};

struct cmdword *get_reboot_reason_map(void)
{
	return reboot_reason_map;
}

u32 get_reboot_reason_map_size(void)
{
	return (u32)ARRAY_SIZE(reboot_reason_map);
}

/*
 * func description:
 *    get exception core str for core id
 * func args:
 *    u64 coreid
 * return value
 *    NULL  error
 *    !NULL core str
 */
char *rdr_get_exception_type(u64 e_exce_type)
{
	int i;

	for (i = 0; (unsigned int)i < get_reboot_reason_map_size(); i++) {
		if (reboot_reason_map[i].num == e_exce_type)
			return (char *)reboot_reason_map[i].name;
	}

	return "UNDEF";
}

enum EXCEPTION_CORE_LIST {
	EXCEPTION_CORE_AP,
	EXCEPTION_CORE_CSUSE,
	EXCEPTION_CORE_CSUPM,
	EXCEPTION_CORE_SF,
	EXCEPTION_CORE_HIFI,
	EXCEPTION_CORE_ISP,
	EXCEPTION_CORE_VPU,
	EXCEPTION_CORE_NPU,
	EXCEPTION_CORE_DPU,
	EXCEPTION_CORE_TEEOS,
	EXCEPTION_CORE_RCSU,
	EXCEPTION_CORE_CLK,
	EXCEPTION_CORE_EXCEPTION_TRACE,
	EXCEPTION_CORE_CSI,
	EXCEPTION_CORE_CSIDMA,
	EXCEPTION_CORE_UNDEF,
};

char *exception_core[RDR_CORE_MAX + 1] = {
	"AP",
	"CSUSE",
	"CSUPM",
	"SF",
	"HIFI",
	"ISP",
	"VPU",
	"NPU",
	"DPU",
	"TEEOS",
	"RCSU",
	"CLK",
	"EXCEPTION_TRACE",
	"CSI",
	"CSIDMA",
	"UNDEF",
};

/*
 * Description:   get description of exception by core id
 * Return:        value:NULL error,!NULL core string
 */
char *rdr_get_exception_core(u64 coreid)
{
	char *core = NULL;

	switch (coreid) {
	case RDR_AP:
		core = exception_core[EXCEPTION_CORE_AP];
		break;
	case RDR_CSUSE:
		core = exception_core[EXCEPTION_CORE_CSUSE];
		break;
	case RDR_CSUPM:
		core = exception_core[EXCEPTION_CORE_CSUPM];
		break;
	case RDR_SF:
		core = exception_core[EXCEPTION_CORE_SF];
		break;
	case RDR_HIFI:
		core = exception_core[EXCEPTION_CORE_HIFI];
		break;
	case RDR_ISP:
		core = exception_core[EXCEPTION_CORE_ISP];
		break;
	case RDR_VPU:
		core = exception_core[EXCEPTION_CORE_VPU];
		break;
	case RDR_NPU:
		core = exception_core[EXCEPTION_CORE_NPU];
		break;
	case RDR_DPU:
		core = exception_core[EXCEPTION_CORE_DPU];
		break;
	case RDR_TEEOS:
		core = exception_core[EXCEPTION_CORE_TEEOS];
		break;
	case RDR_RCSU:
		core = exception_core[EXCEPTION_CORE_RCSU];
		break;
	case RDR_CLK:
		core = exception_core[EXCEPTION_CORE_CLK];
		break;
	case RDR_CSI:
		core = exception_core[EXCEPTION_CORE_CSI];
		break;
	case RDR_CSIDMA:
		core = exception_core[EXCEPTION_CORE_CSIDMA];
		break;
	default:
		core = exception_core[EXCEPTION_CORE_UNDEF];
		break;
	}
	return core;
}

atomic_t bb_in_suspend = ATOMIC_INIT(0);
atomic_t bb_in_reboot = ATOMIC_INIT(0);
atomic_t bb_in_saving = ATOMIC_INIT(0);

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
static int bb_suspend_nb(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		BB_PRINT_DBG("%s: resume +\n", __func__);
		atomic_set(&bb_in_suspend, 0);
		BB_PRINT_DBG("%s: resume -\n", __func__);
		break;

	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		BB_PRINT_DBG("%s: suspend +\n", __func__);
		atomic_set(&bb_in_suspend, 1);
		while (1) {
			if (atomic_read(&bb_in_saving))
				msleep(1000);
			else
				break;
		}
		BB_PRINT_DBG("%s: suspend -\n", __func__);
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
	BB_PRINT_PN("%s: shutdown +\n", __func__);
	atomic_set(&bb_in_reboot, 1);
	while (i--) {
		if (atomic_read(&bb_in_saving))
			msleep(1000);
		else
			break;
		BB_PRINT_DBG("rdr:is saving rdr, wait 1s\n");
	}
	rdr_field_reboot_done();
	BB_PRINT_PN("%s: shutdown -\n", __func__);

	return 0;
}

static rdr_pdev g_rdr_device = {0};
enum RDR_DTS_DATA_INDX {
	MAX_LOGNUM = 0,
	DUMPLOG_TIMEOUT,
	REBOOT_TIMES,
	RDR_DTS_U32_NUMS,
};

struct rdr_dts_prop {
	int indx;
	const char *propname;
	u32 data;
} g_rdr_dts_data[RDR_DTS_U32_NUMS] = {
	{ MAX_LOGNUM,        "rdr-log-max-nums", },
	{ DUMPLOG_TIMEOUT,   "wait-dumplog-timeout", },
	{ REBOOT_TIMES,      "unexpected-max-reboot-times", },
};

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

int rdr_get_dumplog_timeout(void)
{
	return g_rdr_dts_data[DUMPLOG_TIMEOUT].data;
}

u64 rdr_get_logsize(void)
{
	return g_rdr_device.max_logseze;
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
		if (modid >= g_modid_list[i].modid_span_little &&
		    modid <= g_modid_list[i].modid_span_big)
			return g_modid_list[i].modid_str;
	}

	return "error,modid not found";
}

static int rdr_get_property_data_u32(struct device *dev)
{
	u32 value = 0;
	int i, ret;

	if (dev == NULL) {
		BB_PRINT_ERR("[%s], parameter device dev is NULL!\n", __func__);
		return -1;
	}

	for (i = 0; i < RDR_DTS_U32_NUMS; i++) {
		ret = device_property_read_u32(dev, g_rdr_dts_data[i].propname, &value);
		if (ret) {
			BB_PRINT_ERR("[%s], cannot find g_rdr_dts_data[%d],[%s] in dts!\n",
			    __func__, i, g_rdr_dts_data[i].propname);
			return ret;
		}

		g_rdr_dts_data[i].data = value;
		BB_PRINT_DBG("[%s], get %s [0x%x] in dts!\n", __func__,
		    g_rdr_dts_data[i].propname, value);
	}

	return 0;
}

int rdr_common_early_init(struct platform_device *pdev)
{
	int i, ret, len;
	struct device *dev = NULL;
	struct fwnode_handle *bbox_node = NULL;
	const char *prdr_dumpctrl = NULL;
	struct rdr_area_data *rdr_area_data = &g_rdr_device.area_data;
	u64 bbox_data[2];

	rdr_area_data->value = 0;
	dev = &pdev->dev;
	if (device_property_read_u64(dev, "ramlog_addr", &g_rdr_device.ramlog.paddr)) {
		BB_PRINT_DBG("%s: get ramlog address from dts failed... \n", __func__);
		g_rdr_device.ramlog.paddr = 0;
	}

	if (device_property_read_u64(dev, "ramlog_size", &g_rdr_device.ramlog.size)) {
		BB_PRINT_DBG("%s: get ramlog size from dts failed... \n", __func__);
		g_rdr_device.ramlog.size = 0;
	}

	if (g_rdr_device.ramlog.paddr && g_rdr_device.ramlog.size) {
		g_rdr_device.ramlog.vaddr = bbox_vmap(g_rdr_device.ramlog.paddr, g_rdr_device.ramlog.size);
		BB_PRINT_DBG("%s: ramlog address=0x%px \n", __func__, g_rdr_device.ramlog.vaddr);
	}

	bbox_node = fwnode_find_reference(dev->fwnode, "bbox_addr",0);
	if (bbox_node == NULL) {
		BB_PRINT_ERR("[%s], no bbox_addr phandle, %d\n", __func__,device_get_child_node_count(dev));
		return -ENODEV;
	}

	ret = fwnode_property_read_u64_array(bbox_node, "reg", bbox_data, 2);
	if (ret) {
		BB_PRINT_ERR("failed to translate bbox_addr to resource: %d\n",
			     ret);
		return ret;
	}

	g_rdr_device.rdr.paddr = bbox_data[0];
	g_rdr_device.rdr.size = bbox_data[1];

	BB_PRINT_DBG("[%s], get phymem addr [%llx] size [%llx]in dts!\n",
	    __func__, g_rdr_device.rdr.paddr, g_rdr_device.rdr.size);

	if (g_rdr_device.rdr.paddr && g_rdr_device.rdr.size) {
		g_rdr_device.rdr.vaddr = bbox_vmap(g_rdr_device.rdr.paddr, g_rdr_device.rdr.size);
		BB_PRINT_DBG("%s: rdr address=0x%px \n", __func__, g_rdr_device.rdr.vaddr);
	}

	ret = device_property_read_string(dev, "rdr-dumpctl", &prdr_dumpctrl);
	if (ret < 0 || prdr_dumpctrl == NULL || strlen(prdr_dumpctrl) > RDR_DUMPCTRL_LENGTH) {
		BB_PRINT_ERR("[%s], find rdr-dumpctl node fail! [%s]\n", __func__, prdr_dumpctrl);
		return ret;
	}
	BB_PRINT_DBG("[%s], get prdr_dumpctrl [%s] in dts!\n", __func__, prdr_dumpctrl);
	g_rdr_device.nve = 0;
	len = strlen(prdr_dumpctrl);
	for (i = --len; i >= 0; i--) {
		if (prdr_dumpctrl[i] == '1')
			g_rdr_device.nve |= (u64)1 << (unsigned int)(len - i);
	}
	BB_PRINT_DBG("[%s], get nve [0x%llx] in dts!\n", __func__, g_rdr_device.nve);

	ret = device_property_read_u32(dev, "rdr-log-max-size", &g_rdr_device.max_logseze);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr-log-max-size in dts!\n", __func__);
		return ret;
	}
	BB_PRINT_DBG("[%s], get rdr-log-max-size [0x%x] in dts!\n", __func__, g_rdr_device.max_logseze);

	ret = rdr_get_property_data_u32(&pdev->dev);
	if (ret < 0)
		return ret;

	ret = device_property_read_u32(dev, "rdr_area_num", &rdr_area_data->value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_num in dts!\n", __func__);
		return ret;
	}
	BB_PRINT_DBG("[%s], get rdr_area_num [0x%x] in dts!\n", __func__, rdr_area_data->value);

	if (rdr_area_data->value > RDR_CORE_MAX) {
		BB_PRINT_ERR("[%s], invaild core num in dts!\n", __func__);
		return -1;
	}
	ret = device_property_read_u32_array(dev, "rdr_area_sizes", &rdr_area_data->data[0], rdr_area_data->value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_sizes in dts!\n", __func__);
		return ret;
	}
	rdr_set_area_info(0, rdr_area_data->value);
	for (i = 1; i < (int)rdr_area_data->value; i++) {
		rdr_set_area_info(i, rdr_area_data->data[i]);
		BB_PRINT_DBG("[%s], get rdr_area_num[%d]:[0x%x] in dts!\n", __func__, i, rdr_area_data->data[i]);
	}

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
		BB_PRINT_ERR("%s: Failed to register for PM events\n",
		    __func__);
		return -1;
	}
#endif

	bb_reboot_notifier.notifier_call = bb_reboot_nb;
	bb_reboot_notifier.priority = -1;
	if (register_reboot_notifier(&bb_reboot_notifier)) {
		BB_PRINT_ERR("%s: Failed to register for Reboot events\n",
		    __func__);
		return -1;
	}
	return 0;
}

static char g_reboot_reason[RDR_REBOOT_REASON_LEN] = "undef";
static u32 g_reboot_type;
char *rdr_get_reboot_reason(void)
{
	return g_reboot_reason;
}

u32 rdr_get_reboot_type(void)
{
	return g_reboot_type;
}

static int __init early_parse_reboot_reason_cmdline(char *reboot_reason_cmdline)
{
	int i;

	if (reboot_reason_cmdline == NULL) {
		BB_PRINT_ERR("[%s:%d]: reboot_reason_cmdline is null\n", __func__, __LINE__);
		return -1;
	}

	memset(g_reboot_reason, 0x0, RDR_REBOOT_REASON_LEN);

	memcpy(g_reboot_reason, reboot_reason_cmdline, RDR_REBOOT_REASON_LEN - 1);

	for (i = 0; (u32)i < get_reboot_reason_map_size(); i++) {
		if (!strncmp((char *)reboot_reason_map[i].name, g_reboot_reason, RDR_REBOOT_REASON_LEN)) {
			g_reboot_type = reboot_reason_map[i].num;
			break;
		}
	}
	BB_PRINT_PN("[%s][%s][%u]\n", __func__, g_reboot_reason, g_reboot_type);
	return 0;
}

early_param("reboot_reason", early_parse_reboot_reason_cmdline);

void *bbox_vmap(phys_addr_t paddr, size_t size)
{
	int i;
	void *vaddr = NULL;
	pgprot_t pgprot;
	unsigned long offset;
	int pages_count;
	struct page **pages = NULL;

	offset = paddr & ~PAGE_MASK;
	paddr &= PAGE_MASK;
	pages_count = PAGE_ALIGN(size + offset) / PAGE_SIZE;

	pages = kzalloc(sizeof(struct page *) * pages_count, GFP_KERNEL);
	if (pages == NULL)
		return NULL;

	pgprot = pgprot_writecombine(PAGE_KERNEL);

	for (i = 0; i < pages_count; i++)
		*(pages + i) = phys_to_page((uintptr_t)(paddr + PAGE_SIZE * i));

	vaddr = vmap(pages, pages_count, VM_MAP, pgprot);
	kfree(pages);
	if (vaddr == NULL)
		return NULL;

	return offset + (char *)vaddr;
}

void *rdr_bbox_map(phys_addr_t paddr, size_t size)
{
	void *vaddr = NULL;

	if (paddr < g_rdr_device.rdr.paddr || !size || ((paddr + size) < paddr) ||
	   (paddr + size) > (g_rdr_device.rdr.paddr + g_rdr_device.rdr.size)) {
		BB_PRINT_ERR("Error BBox memory\n");
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
	if (!pfn_valid(g_rdr_device.rdr.paddr >> PAGE_SHIFT))
		iounmap((void __iomem *)vaddr);
}
EXPORT_SYMBOL(rdr_bbox_unmap);

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

	BB_PRINT_START();

	if (logpath == NULL ||
	   (strlen(logpath) + strlen(BBOX_SAVE_DONE_FILENAME) + 1) > PATH_MAXLEN) {
		BB_PRINT_ERR("logpath is invalid\n");
		return;
	}

	BB_PRINT_PN("logpath is [%s], step is [%u]\n", logpath, step);
	if (step == BBOX_SAVE_STEP_DONE) {
		/* combine the absolute path of the done file as a parameter of sys_mkdir */
		memset(path, 0, PATH_MAXLEN);
		len = strlen(logpath);
		if (len >= PATH_MAXLEN) {
			BB_PRINT_ERR("[%s:%d]: memcpy err\n]", __func__, __LINE__);
			return;
		}
		memcpy(path, logpath, len);

		strncat(path, BBOX_SAVE_DONE_FILENAME, ((PATH_MAXLEN - 1) - strlen(path)));
		if (strncmp(path, PATH_ROOT, strlen(PATH_ROOT))) {
			BB_PRINT_ERR("[%s:%d]: path [%s] err\n]", __func__, __LINE__, path);
			return;
		}

		/* create a done file under the timestamp directory */
		file = filp_open(path, O_CREAT | O_WRONLY, FILE_LIMIT);
		if (IS_ERR(file)) {
			BB_PRINT_ERR("create [%s] error \n", path);
			return;
		}
		filp_close(file, NULL);

		/*
		 * according to the permission requirements,
		 * the hisi_logs directory and subdirectory group are adjusted to root-system
		 */
		ret = (int)rdr_chown((const char __user *)path, ROOT_UID, SYSTEM_GID, false);
		if (ret)
			BB_PRINT_ERR("chown %s uid [%d] gid [%d] failed err [%d]!\n",
			     PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
	}

	BB_PRINT_END();
}

/*
 * Description:  save reboot times to specified memory
 */
void rdr_record_reboot_times2mem(void)
{
	struct rdr_struct_s *pbb = NULL;

	BB_PRINT_START();
	pbb = rdr_get_pbb();
	pbb->top_head.reserve = RDR_UNEXPECTED_REBOOT_MARK_ADDR;
	BB_PRINT_END();
}

/*
 * Description:   reset the file saving reboot times
 */
void rdr_reset_reboot_times(void)
{
	struct file *fp = NULL;
	ssize_t length;
	char buf;

	BB_PRINT_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, FILE_LIMIT);
	if (IS_ERR(fp)) {
		BB_PRINT_ERR("rdr:%s(),open %s fail\n", __func__,
			    RDR_REBOOT_TIMES_FILE);
		return;
	}
	buf = 0;
	vfs_llseek(fp, 0L, SEEK_SET);
	length = kernel_write(fp, &buf, sizeof(buf), &(fp->f_pos));
	if (length == sizeof(buf))
		vfs_fsync(fp, 0);

	filp_close(fp, NULL);
	BB_PRINT_END();
}

/*
 * Description:   write the enter reason of erecovery to /cache/recovery/last_erecovery_entry
 */
void rdr_record_erecovery_reason(void)
{
	struct file *fp = NULL;
	ssize_t length;
	const char *e_reason = "erecovery_enter_reason:=2015\n";

	BB_PRINT_START();

	fp = filp_open(RDR_ERECOVERY_REASON_FILE, O_CREAT | O_RDWR, FILE_LIMIT);
	if (IS_ERR(fp)) {
		BB_PRINT_ERR("rdr:%s(),open %s first fail,error No. %pK\n", __func__, RDR_ERECOVERY_REASON_FILE, fp);
		rdr_create_dir("/cache/recovery/");
		fp = filp_open(RDR_ERECOVERY_REASON_FILE, O_CREAT | O_RDWR | O_TRUNC, FILE_LIMIT);
		if (IS_ERR(fp)) {
			BB_PRINT_ERR("rdr:%s(),open %s second fail,error No. %pK\n",
			    __func__, RDR_ERECOVERY_REASON_FILE, fp);
			return;
		}
	}
	vfs_llseek(fp, 0L, SEEK_SET);
	length = kernel_write(fp, e_reason, strlen(e_reason) + 1, &(fp->f_pos));
	if (length == (strlen(e_reason) + 1))
		vfs_fsync(fp, 0);

	filp_close(fp, NULL);
	BB_PRINT_END();
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

	BB_PRINT_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, FILE_LIMIT);
	if (IS_ERR(fp)) {
		BB_PRINT_ERR("rdr:%s(),open %s fail\n", __func__,
			   RDR_REBOOT_TIMES_FILE);
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
	BB_PRINT_END();
	return buf;
}

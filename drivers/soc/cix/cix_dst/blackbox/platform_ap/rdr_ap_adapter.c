// SPDX-License-Identifier: GPL-2.0-only
/*
 * rdr_ap_adapter.c
 *
 * Based on the RDR framework, adapt to the AP side to implement resource
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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

#include <linux/soc/cix/mntn_dump.h>
#include <linux/syscalls.h>
#include <linux/cacheflush.h>
#include <linux/console.h>
#include <linux/kmsg_dump.h>
#include <linux/reboot.h>
#include <mntn_subtype_exception.h>
#include <mntn_dump_interface.h>
#include <linux/soc/cix/cix_ap2se_ipc.h>
#include "include/rdr_ap_adapter.h"
#include "../rdr_inner.h"
#include "../rdr_print.h"

static struct ap_eh_root *g_eh_ap;
static int g_rdr_ap_init;

int ap_prop_table_init(struct device *dev, struct property_table *table,
		       u32 table_size)
{
	int ret, res;
	u32 i = 0;

	BB_PR_START();
	for (i = 0; i < table_size; i++) {
		/* empty size string, set size zero */
		if (unlikely(!table[i].prop_name)) {
			table[i].size = 0;
			continue;
		}
		ret = device_property_read_u32(dev, table[i].prop_name,
					       &table[i].size);
		if (ret) {
			/* no dts property, set size zero */
			BB_ERR("cannot find %s in dts!\n", table[i].prop_name);
			table[i].size = 0;
			continue;
		}
		/*As long as one initialization is successful, it means ok.*/
		res = 0;
		BB_DBG("%s=0x%x get from dts\n", table[i].prop_name,
		       table[i].size);
	}
	BB_PR_END();
	return res;
}

static void print_debug_info(void)
{
	BB_PN("=================struct ap_eh_root================");
	BB_PN("dump_magic [0x%x]\n", g_eh_ap->dump_magic);
	BB_PN("version [%s]\n", g_eh_ap->version);
	BB_PN("slice [%llu]\n", g_eh_ap->slice);
	BB_PN("enter_times [0x%x]\n", g_eh_ap->enter_times);

	regsdump_debug_info();
}

static void rdr_ap_print_all_dump_addrs(void)
{
	if (IS_ERR_OR_NULL(g_eh_ap)) {
		BB_ERR("g_rdr_ap_root [0x%px] is invalid\n", g_eh_ap);
		return;
	}

	regsdump_debug_info();
	hook_debug_info();
	moddump_debug_info();
}

// need implement according to platform (vincent).
u64 get_32k_abs_timer_value(void)
{
	return 0;
}

void *get_addr_from_root(struct ap_eh_root *ehroot, void *addr)
{
	void *vaddr = NULL;

	if (rdr_bch_checkout((void *)ehroot, ROOT_CHECK_SIZE, ehroot->ecc,
			     sizeof(ehroot->ecc))) {
		BB_PN("ehroot checkout fail\n");
		return NULL;
	}

	vaddr = GET_ADDR_FROM_EHROOT(ehroot, addr);
	return vaddr;
}

static bool modid_possible_hardlock(u32 modid)
{
	if (modid == MODID_AP_PANIC_RCUSTALL ||
	    modid == MODID_AP_PANIC_HARDLOCKUP || modid == MODID_AP_AWDT_BL31)
		return true;
	return false;
}

static void rdr_ap_dump_no_irq(u32 modid, u32 etype, u64 coreid, char *logpath)
{
	BB_PN("ap_trace_hook_uninstall start!\n");
	/*
	 * The trace hook utilizes static-key and internally invokes tlbi operations.
	 * A hardlock can cause the core executing tlbi to also experience a hardlock.
	 */
	ap_trace_hook_uninstall();
}

static void rdr_ap_dump(u32 modid, u32 etype, u64 coreid, char *log_path)
{
	BB_PN("begin\n");
	BB_PN("modid[%x], etype[%x], coreid[%llx], log_path[%s]\n", modid,
	      etype, coreid, log_path);

	if (!g_rdr_ap_init) {
		BB_ERR("rdr_hisi_ap_adapter is not ready\n");
		return;
	}

	if (modid == RDR_MODID_AP_ABNORMAL_REBOOT ||
	    modid == BBOX_MODID_LAST_SAVE_NOT_DONE) {
		/*exception boot*/
		BB_PN("RDR_MODID_AP_ABNORMAL_REBOOT\n");
		pstore_dump_mount();
		return;
	}

	if (modid_possible_hardlock(modid)) {
		/*dump amu pwr clk...*/
		cix_ap2se_ipc_send(FFA_REQ_AP_HARDLOCK, NULL, 0, 0);
	}

	if (!g_eh_ap)
		goto out;

	g_eh_ap->slice = get_32k_abs_timer_value();

	BB_PN("regs_dump start!\n");
	regs_dump();
	g_eh_ap->enter_times++;
	BB_PN("save_module_dump_mem start!\n");
	save_module_dump_mem();
	ap_suspend_dump(modid, etype);
	print_debug_info();
out:
	BB_PN("exit!\n");
}

static void rdr_ap_callback(u32 modid, u32 etype, u64 coreid)
{
	if (modid == RDR_MODID_AP_ABNORMAL_REBOOT ||
	    modid == BBOX_MODID_LAST_SAVE_NOT_DONE) {
		return;
	}
	if (check_himntn(HIMNTN_KERNEL_DUMP_ENABLE)) {
		BB_PN("install start\n");
		if (ap_trace_hook_install())
			BB_ERR("install err\n");
	} else {
		BB_PN("def install start\n");
		ap_def_trace_hook_install();
	}
}

/*
 * Description : Reset function of the AP when an exception occurs
 */
static void rdr_ap_reset(u32 modid, u32 etype, u64 coreid)
{
	if (!in_atomic() && !irqs_disabled() && !in_irq())
		ksys_sync();

	if (etype != AP_PANIC) {
		BB_PN("etype is not panic\n");
		dump_stack();
		preempt_disable();
		smp_send_stop();
	}

	console_flush_on_panic(CONSOLE_FLUSH_PENDING);
	kmsg_dump(KMSG_DUMP_PANIC);
#ifdef CONFIG_PLAT_KERNELDUMP
	kd_save_state_shutdown();
#endif
	__flush_dcache_all();

	/* HIMNTN_PANIC_INTO_LOOP will disbale ap reset */
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) == 1) {
		do {
		} while (1);
	}
	machine_restart(NULL);
}

/*
 * The clear text printing for the ap head of core AP
 *
 * func args:
 * @dir_path: the file directory of saved clear text
 * @log_addr: the start address of reserved memory for specified core
 * @log_len: the length of reserved memory for specified core
 *
 * return value
 * 0 success
 * otherwise failure
 *
 */
static int ap_head_cleartext(const char *dir_path, u64 log_addr, u32 log_len)
{
	struct file *fp = NULL;
	struct ap_eh_root *ap_root = NULL;
	bool error = false;

	if (!dir_path) {
		BB_ERR("parameter is NULL\n");
		return -1;
	}
	if (unlikely(log_len < sizeof(*ap_root))) {
		BB_ERR("error:log_len %u not enough.\n", log_len);
		return -1;
	}

	/* get the file descriptor from the specified directory path */
	fp = bbox_cleartext_get_filep(dir_path, "AP_HEAD.txt");
	if (IS_ERR_OR_NULL(fp)) {
		BB_ERR("error:fp 0x%pK\n", fp);
		return -1;
	}

	ap_root = (struct ap_eh_root *)(uintptr_t)(log_addr);
	error = false;
	ap_root->version[PRODUCT_VERSION_LEN - 1] = '\0';
	ap_root->device_id[PRODUCT_DEVICE_LEN - 1] = '\0';

	rdr_cleartext_print(
		fp, &error,
		"=================struct ap_eh_root START================\n");
	STRUCT_PRINT(ap_root, dump_magic, "0x%x");
	STRUCT_PRINT(ap_root, device_id, "%s");
	STRUCT_PRINT(ap_root, version, "%s");
	STRUCT_PRINT(ap_root, bbox_version, "0x%llx");
	STRUCT_PRINT(ap_root, enter_times, "0x%x");
	STRUCT_PRINT(ap_root, slice, "%llu");
	rdr_cleartext_print(
		fp, &error,
		"=================struct ap_eh_root END--================\n");

	/* For the formal commercial version, hook trace&last task trace& is closed */
	/* the cleaning of specified file descriptor */
	bbox_cleartext_end_filep(fp);

	if (unlikely(error == true))
		return -1;

	return 0;
}

static int rdr_ap_cleartext_print(const char *dir_path, u64 log_addr,
				  u32 log_len)
{
	bool is_last = rdr_log_save_is_last();
	struct ap_eh_root *root = NULL;

	if (is_last) {
		root = (void *)log_addr;
		if (IS_ERR_OR_NULL(root))
			return -EINVAL;
		rdr_safemem_pool_reinit(&root->pool);
	}

	if (unlikely(ap_head_cleartext(dir_path, log_addr, log_len))) {
		BB_ERR("cleartext_ap_head error\n");
		return -1;
	}

	if (unlikely(ap_hook_cleartext(dir_path, log_addr, log_len))) {
		BB_ERR("ap_hook_cleartext error\n");
		return -1;
	}

	if (unlikely(ap_pstore_cleartext(dir_path, log_addr, log_len)))
		BB_PN("ap_pstore_cleartext error\n");

	if (unlikely(ap_laststack_cleartext(dir_path, log_addr, log_len)))
		BB_PN("ap_stack_cleartext error\n");

	if (unlikely(ap_suspend_cleartext(dir_path, log_addr, log_len)))
		BB_PN("ap_stack_cleartext error\n");

	return 0;
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int rdr_ap_register_core(void)
{
	struct rdr_module_ops_pub s_soc_ops;
	struct rdr_register_module_result retinfo;
	int ret;
	u64 coreid = RDR_AP;
	void *vaddr = NULL;

	s_soc_ops.ops_dump = rdr_ap_dump;
	s_soc_ops.ops_dump_no_irq = rdr_ap_dump_no_irq;
	s_soc_ops.ops_reset = rdr_ap_reset;
	s_soc_ops.ops_callback = rdr_ap_callback;

	ret = rdr_register_module_ops(coreid, &s_soc_ops, &retinfo);
	if (ret < 0) {
		BB_ERR("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	ret = rdr_register_cleartext_ops(coreid, rdr_ap_cleartext_print);
	if (ret < 0) {
		BB_ERR("rdr_register_cleartext_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	vaddr = rdr_bbox_map(retinfo.log_addr, retinfo.log_len);
	if (!vaddr) {
		BB_ERR("[%s] map fail\n", rdr_get_core_name_by_core(coreid));
		return -1;
	}

	g_eh_ap = vaddr;
	memset(g_eh_ap, 0, retinfo.log_len);
	g_eh_ap->mem.paddr = retinfo.log_addr;
	g_eh_ap->mem.size = retinfo.log_len;
	g_eh_ap->mem.vaddr = vaddr;

	BB_DBG("addr=0x%llx, vaddr=%px, len=0x%llx\n", g_eh_ap->mem.paddr,
	       vaddr, g_eh_ap->mem.size);
	return ret;
}

static int rdr_ap_dump_init(struct platform_device *pdev)
{
	int ret = 0;

	BB_PR_START();

	ret = regsdump_init(pdev, &g_eh_ap->pool);
	if (ret) {
		BB_ERR("io_resources_init failed!\n");
		return ret;
	}

	ret = ap_hook_init(pdev, &g_eh_ap->pool);
	if (ret) {
		BB_ERR("ap_hook_init failed!\n");
		return ret;
	}

	BB_DBG("ap_dump_buffer_init start\n");
	ret = module_dump_init(pdev, &g_eh_ap->pool);
	if (ret) {
		BB_ERR("ap_dump_buffer_init failed!\n");
		return ret;
	}

	pstore_dump_init(pdev, &g_eh_ap->pool);
	stack_dump_init(pdev, &g_eh_ap->pool);
	suspend_dump_init(pdev, &g_eh_ap->pool);

	rdr_ap_print_all_dump_addrs();
	BB_PR_END();
	return ret;
}

static int ap_eh_head_init(void)
{
	struct bbox_mem mem;
	u64 size;

	g_eh_ap->bbox_version = BBOX_VERSION;
	g_eh_ap->dump_magic = AP_DUMP_MAGIC;

	size = ROOT_HEAD_SIZE - offsetof(struct ap_eh_root, pool);
	mem.size = g_eh_ap->mem.size - ROOT_HEAD_SIZE;
	mem.vaddr = g_eh_ap->mem.vaddr + ROOT_HEAD_SIZE;
	mem.paddr = g_eh_ap->mem.paddr + ROOT_HEAD_SIZE;

	if (rdr_safemem_pool_init(&g_eh_ap->pool, "apcore", size, &mem, true))
		return -1;

	return rdr_bch_encode((void *)g_eh_ap, ROOT_CHECK_SIZE, g_eh_ap->ecc,
			      sizeof(g_eh_ap->ecc));
}

/*
 * Description : Obtains the initialization status
 */
bool rdr_get_ap_init_done(void)
{
	return g_rdr_ap_init == 1;
}

static const struct of_device_id rdr_ap_of_match[] = {
	{ .compatible = "rdr,rdr_ap_adapter" },
	{}
};

static int rdr_hisiap_probe(struct platform_device *pdev)
{
	int ret;

	BB_PR_START();

	ret = rdr_ap_register_core();
	if (ret) {
		BB_ERR("rdr_ap_register_core fail, ret = [%d]\n", ret);
		return ret;
	}
	ap_eh_head_init();
	ret = rdr_ap_dump_init(pdev);
	if (ret) {
		BB_ERR("rdr_ap_dump_init fail, ret = [%d]\n", ret);
		return -1;
	}
	ret = rdr_exception_init();
	if (ret) {
		BB_ERR("rdr_exception_init fail, ret = [%d]\n", ret);
		return -1;
	}
	dcache_clean_poc((u64)g_eh_ap, ((u64)g_eh_ap) + sizeof(*g_eh_ap));
	g_rdr_ap_init = 1;
	BB_PR_END();

	return 0;
}

static int rdr_hisiap_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rdr_ap_driver = {
	.driver		= {
		.name			= "rdr ap driver",
		.of_match_table		= rdr_ap_of_match,
	},
	.probe		= rdr_hisiap_probe,
	.remove		= rdr_hisiap_remove,
};

/*
 * Description : Initialization Function
 */
int __init rdr_hisiap_init(void)
{
	platform_driver_register(&rdr_ap_driver);

	return 0;
}

static void __exit rdr_hisiap_exit(void)
{
	platform_driver_unregister(&rdr_ap_driver);
}

module_init(rdr_hisiap_init);
module_exit(rdr_hisiap_exit);

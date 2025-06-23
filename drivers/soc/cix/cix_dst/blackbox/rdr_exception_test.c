// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 */

#include <linux/module.h>
#include <mntn_public_interface.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>

static struct delayed_work test_wq;
static struct delayed_work test_ap_wq;

#ifdef TEST_RDR_FILE_MAX_SIZE
static int g_count = 0;
#endif

static int module_test_rdr_dump(void *dump_addr, unsigned int size)
{
	struct rdr_exception_info_s *info = NULL;
	unsigned int bufsize = min(size, (u32)SZ_4K);

	pr_err("into %s, addr:%px, size:0x%x\n", __func__, dump_addr, size);
	info = rdr_get_exce_info();
	if (!info) {
		pr_err("%s, rdr_get_exce_info failed!\n", __func__);
		return -1;
	}
	pr_info("%s, modid = 0x%x \n", __func__, info->e_modid);

	memset(dump_addr, 0x5A, bufsize);

	return 0;
}

static int module_dump_test_init(void)
{
	pr_err("into %s\n", __func__);
	(void)register_module_dump_mem_func(
			module_test_rdr_dump, "test", MODU_TEST);
	return 0;
}

static void test_exception_work(struct work_struct *work)
{
	pr_info("rdr test exception triggered... \n");
	rdr_system_error(MODID_AP_S_TEST, 0, 0);

#ifdef TEST_RDR_FILE_MAX_SIZE
	if (g_count++ < 7)
		schedule_delayed_work(&test_wq, msecs_to_jiffies(10000));
#endif
}

static void test_exception_ap_work(struct work_struct *work)
{
	pr_info("rdr test ap exception triggered... \n");
	rdr_syserr_process_for_ap(MODID_AP_S_PANIC, 0, 0);
}

static int __init rdr_exception_test(void)
{
	INIT_DELAYED_WORK(&test_wq, test_exception_work);
	INIT_DELAYED_WORK(&test_ap_wq, test_exception_ap_work);

	module_dump_test_init();

	schedule_delayed_work(&test_wq, msecs_to_jiffies(20000));

#ifndef TEST_RDR_FILE_MAX_SIZE
	schedule_delayed_work(&test_ap_wq, msecs_to_jiffies(100000));
#endif
	return 0;
}

module_init(rdr_exception_test);
MODULE_LICENSE("GPL");

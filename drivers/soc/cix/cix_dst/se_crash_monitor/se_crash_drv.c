// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025
 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/platform_device.h>
#include <linux/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_pub.h>
#include "se_crash.h"
#include <linux/soc/cix/cix_ap2se_ipc.h>

#define SE_MEM_MAX_NUM 32
#define SE_EXCEPTION_ADDR_MAGIC 0xbab1face
#define AP_USE (2)

struct se_crash_info {
	u32 flag;
	u32 num;
	struct se_mem_info info[SE_MEM_MAX_NUM];
	char data[];
};

struct se_crash_mem {
	u64 paddr;
	void *vaddr;
	u32 size;
};

struct se_crash_dev {
	struct platform_device *pdev;
	struct delayed_work dwork;
	atomic_t running;
	struct se_crash_mem mem;
};

static struct se_crash_dev g_se_crash_dev;

static struct rdr_exception_info_s g_se_einfo[] = {
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_CSUSE, RDR_CSUSE,
			       RDR_CSUSE, CSUSE_EXCEPTION,
			       CSUSE_EXCEPTION_WDT, "csuse wdt", 0, NULL),
	DEF_EXCE_STRUCT_SINGLE(
		RDR_WARN, RDR_REBOOT_NO, RDR_CSUSE, 0, RDR_CSUSE,
		CSUSE_EXCEPTION, CSUSE_EXCEPTION_TEST, "csuse test",
		RDR_SAVE_CSUSE | RDR_SAVE_BL31 | RDR_SAVE_DMESG, NULL),
};

static struct se_cleartext se_cleartext_info[] = {
	{ SE_MEM_SE_WDT, se_wdt_cleartext },
	{ SE_MEM_PM_WDT, pm_wdt_cleartext },
	{ SE_MEM_AMU, se_amu_cleartext },
	{ SE_MEM_POWER, se_power_cleartext },
	{ SE_MEM_CLK, se_clk_cleartext },
	{ SE_MEM_DDR, se_ddr_cleartext },
};

static struct se_mem_info *se_crash_get_mem(struct se_crash_info *crash,
					    char *name)
{
	for (int i = 0; i < crash->num; i++) {
		if (!strcmp(name, crash->info[i].name))
			return &crash->info[i];
	}

	return NULL;
}

static uint32_t crc32(uint8_t *data, uint16_t length)
{
	uint8_t i;
	uint32_t crc = 0xffffffff; // Initial value
	while (length--) {
		crc ^= *data++; // crc ^= *data; data++;
		for (i = 0; i < 8; ++i) {
			if (crc & 1)
				crc = (crc >> 1) ^
				      0xEDB88320; // 0xEDB88320= reverse 0x04C11DB7
			else
				crc = (crc >> 1);
		}
	}
	return ~crc;
}

static void rdr_se_exception_work(struct work_struct *work)
{
}

static void rdr_se_dump(u32 modid, u32 etype, u64 coreid, char *log_path)
{
	u64 start = (u64)g_se_crash_dev.mem.vaddr;
	struct se_crash_info *info = g_se_crash_dev.mem.vaddr;

	dcache_inval_poc(start, start + g_se_crash_dev.mem.size);
	info->flag = AP_USE;
	dcache_clean_poc(start, start + 4);
}

static int se_cleartext(const char *log_dir_path, u64 log_addr, u32 log_len)
{
	struct se_cleartext *text_info;
	struct se_mem_info *mem;
	struct se_crash_info *crash = NULL, *src = (void *)log_addr;
	int ret = 0;
	uint *crc, rcrc;

	crash = kzalloc(log_len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(crash)) {
		src->flag = 0;
		dcache_clean_poc(log_addr, log_addr + log_len);
		return -1;
	}
	memcpy(crash, src, log_len);
	memset(src, 0, log_len);
	dcache_clean_poc(log_addr, log_addr + log_len);

	for (int i = 0; i < ARRAY_SIZE(se_cleartext_info); i++) {
		text_info = &se_cleartext_info[i];
		mem = se_crash_get_mem(crash, text_info->name);
		if (IS_ERR_OR_NULL(mem))
			continue;
		DST_PN("%s: offset: 0x%x, size: 0x%x\n", mem->name, mem->offset,
		       mem->size);
		crc = (void *)((u64)crash + mem->offset + mem->size -
			       sizeof(uint32_t));
		rcrc = crc32((void *)((u64)crash + mem->offset),
			     mem->size - sizeof(uint32_t));
		if (*crc != rcrc) {
			DST_PN("crc error, 0x%x, 0x%x\n", *crc, rcrc);
			continue;
		}
		if (IS_ERR_OR_NULL(text_info->ops))
			continue;
		ret = text_info->ops(log_dir_path, (u64)crash + mem->offset,
				     mem->size - sizeof(uint32_t));
		if (ret)
			DST_ERR("cleartext %s fail, ret = [%d]\n",
				text_info->name, ret);
	}
	kfree(crash);

	return 0;
}

static void rdr_se_unregister_exception(void)
{
	unsigned int i;
	u32 ret;

	for (i = 0; i < ARRAY_SIZE(g_se_einfo); i++) {
		ret = rdr_unregister_exception(g_se_einfo[i].e_modid);
		if (ret)
			DST_ERR("rdr_unregister_exception %d fail, ret = [%u]\n",
				i, ret);
	}
}

static void rdr_se_register_exception(void)
{
	unsigned int i;
	u32 ret;

	for (i = 0; i < ARRAY_SIZE(g_se_einfo); i++) {
		DST_DBG("register exception:%u\n", g_se_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_se_einfo[i]);
		if (!ret)
			DST_ERR("rdr_register_exception %d fail, ret = [%u]\n",
				i, ret);
	}
}

static int se_crash_set_exception_addr(struct se_crash_dev *sdev)
{
	u32 msg[3];
	int ret = 0;

	msg[0] = sdev->mem.paddr >> 32;
	msg[1] = sdev->mem.paddr & 0xffffffff;
	msg[2] = SE_EXCEPTION_ADDR_MAGIC;

	ret = cix_ap2se_ipc_send(FFA_SET_EXCEPTION_ADDR,
		(char *)msg, sizeof(msg), 0);
	if (ret < 0) {
		DST_ERR("cix_ap2se_ipc_send fail, ret = [%d]\n", ret);
		return -EIO;
	}

	return 0;
}

static int rdr_se_unregister_core(struct se_crash_dev *sdev)
{
	int ret;

	ret = rdr_unregister_module_ops(RDR_CSUSE);
	if (ret)
		DST_ERR("rdr_unregister_module_ops fail, ret = [%d]\n", ret);

	return ret;
}

static int rdr_se_register_core(struct se_crash_dev *sdev)
{
	struct rdr_module_ops_pub se_ops;
	struct rdr_register_module_result retinfo;
	int ret;

	se_ops.ops_dump = rdr_se_dump;
	se_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(RDR_CSUSE, &se_ops, &retinfo);
	if (ret) {
		DST_ERR("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	sdev->mem.paddr = retinfo.log_addr;
	sdev->mem.size = retinfo.log_len;
	sdev->mem.vaddr = rdr_bbox_map(sdev->mem.paddr, sdev->mem.size);
	memset(sdev->mem.vaddr, 0, sdev->mem.size);
	dcache_clean_poc((u64)sdev->mem.vaddr,
			 (u64)sdev->mem.vaddr + sdev->mem.size);

	ret = rdr_register_cleartext_ops(RDR_CSUSE, se_cleartext);
	if (ret) {
		DST_ERR("rdr_register_cleartext_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	/*set se exception addr*/
	ret = se_crash_set_exception_addr(sdev);

	return ret;
}

static void se_crash_mbox_rx_callback(char *inbuf, size_t len)
{
	rdr_system_error(MODID_CSUSE_EXCEPTION_WDT, 0, 0);
}

static void pm_crash_mbox_rx_callback(char *inbuf, size_t len)
{
	rdr_system_error(MODID_CSUPM_EXCEPTION_RES, 0, 0);
}

static void test_crash_mbox_rx_callback(char *inbuf, size_t len)
{
	rdr_system_error(MODID_CSUSE_EXCEPTION_TEST, 0, 0);
}

static void se_crash_register_callbak(void)
{
	cix_ap2se_register_rx_cbk(FFA_SRC_SE_CRASH, se_crash_mbox_rx_callback);
	cix_ap2se_register_rx_cbk(FFA_SRC_PM_CRASH, pm_crash_mbox_rx_callback);
	cix_ap2se_register_rx_cbk(FFA_SRC_TEST_CRASH, test_crash_mbox_rx_callback);
}

static const struct of_device_id se_crash_of_match[] = {
	{ .compatible = "cix,se_pm_crash" },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, se_crash_of_match);

static int se_crash_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct se_crash_dev *sdev = &g_se_crash_dev;

	sdev->pdev = pdev;
	platform_set_drvdata(pdev, sdev);

	atomic_set(&sdev->running, 0);
	INIT_DELAYED_WORK(&sdev->dwork, rdr_se_exception_work);

	ret = rdr_se_register_core(sdev);
	if (ret)
		goto free_ch;

	rdr_se_register_exception();
	se_crash_register_callbak();

	dev_info(&pdev->dev, "se_crash_probe success\n");

free_ch:
	return ret;
}

static int se_crash_remove(struct platform_device *pdev)
{
	struct se_crash_dev *sdev = platform_get_drvdata(pdev);

	rdr_se_unregister_exception();
	rdr_se_unregister_core(sdev);

	return 0;
}

static struct platform_driver sp_crash_driver = {
	.probe = se_crash_probe,
	.remove = se_crash_remove,
	.driver = {
		.name = "cix-se-pm-crash",
		.of_match_table = se_crash_of_match,
	},
};
module_platform_driver(sp_crash_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CIX SE crash monitor Driver");
MODULE_AUTHOR("Vimoon Zheng <Vimoon.Zheng@cixtech.com>");

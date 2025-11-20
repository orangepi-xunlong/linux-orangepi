/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020-2025, Allwinnertech
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* #define DEBUG */
#include <linux/arm-smccc.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/remoteproc.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_wakeirq.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <linux/kthread.h>
#endif
#include <linux/clk.h>

#include "remoteproc_internal.h"
#include "sunxi_rproc_boot.h"
#include "remoteproc_elf_helpers.h"

#include "sunxi_rproc_standby.h"
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
#include "sunxi_share_interrupt/sunxi_share_interrupt.h"
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
#include "sunxi_rproc_subdev.h"
#endif

#include "sunxi_rproc_firmware.h"
#include <linux/remoteproc/sunxi_remoteproc.h>
#include <linux/remoteproc/sunxi_rproc_rsc.h>

#define SUNXI_RPROC_VERSION "2.4.7"

#define MBOX_NB_VQ		2

#define AW_MEM_FW_REGION_PROPERTY_NAME "fw-region"
#define USING_KERNEL_FW_PROPERTY_NAME "using-kernel-fw"
#define RPROC_COREDUMP_PROPERTY_NAME "coredump"

#define RPROC_NMI_AVAILABLE_PROPERTY_NAME "nmi-available"
#define RPROC_WAIT_NMI_COMPLETE_TIMEOUT_PROPERTY_NAME "wait-nmi-complete-timeout"
#define RPROC_DEFAULT_WAIT_NMI_COMPLETE_TIMEOUT 50 /* unit: ms */

#define RPROC_MAX_RSC_TABLE_SIZE 4096

static LIST_HEAD(sunxi_rproc_list);
struct sunxi_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	struct task_struct *rx_daemon;
	wait_queue_head_t rx_wait;
#else
	struct work_struct vq_work;
#endif
	int vq_id;
};

struct sunxi_rproc {
	struct sunxi_rproc_priv *rproc_priv;  /* dsp/riscv private resources */
	struct sunxi_rproc_standby *rproc_standby;

	struct sunxi_mbox mb;
	struct workqueue_struct *workqueue;
	struct list_head list;
	struct rproc *rproc;

	struct device_node *np;
	/* whether using the firmware which is provided by kernel remoteproc framework(request_firmware) */
	int is_using_kernel_fw;
	/* private firmware */
	const struct firmware *fw;
	/* members below is used to save resource table info when we release private firmware */
	uint32_t fw_rsc_table_size;
	uint8_t fw_rsc_table_data[RPROC_MAX_RSC_TABLE_SIZE];

#ifdef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
	int is_zero_copy_mem_fw_active;
	const char *zero_copy_mem_fw_name;
#endif

	void __iomem *rsc_table_va;
	bool is_booted;
	/* only boot on remote core without rsc table */
	bool only_boot;
	bool skip_shutdown;
	char *name;
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	struct sunxi_rproc_subdev subdev;
#endif
	struct fw_rsc_misc_rsc_data *misc_rsc_data;

	int is_nmi_available;
	uint32_t wait_nmi_complete_timeout;
};

int simulator_debug;
module_param(simulator_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(simulator_debug, "Debug for simulator");

static int sunxi_rproc_pa_to_da(struct rproc *rproc, phys_addr_t pa, u64 *da)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	/*
	 * Maybe there are multiple DAs corresponding to one PA.
	 * Here we only return the first matching one in the map table.
	 */
	for (i = 0; i < chip->rproc_priv->mem_maps_cnt; i++) {
		map = &chip->rproc_priv->mem_maps[i];
		if (pa < map->pa || pa >= map->pa + map->len)
			continue;
		*da = pa - map->pa + map->da;
		dev_dbg(dev, "translate pa %pa to da 0x%llx\n", &pa, *da);
		return 0;
	}

	dev_err(dev, "Failed to translate pa %pa to da\n", &pa);
	return -EINVAL;
}

static int sunxi_rproc_da_to_pa(struct rproc *rproc, u64 da, phys_addr_t *pa)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	for (i = 0; i < chip->rproc_priv->mem_maps_cnt; i++) {
		map = &chip->rproc_priv->mem_maps[i];
		if (da < map->da || da >= map->da + map->len)
			continue;
		*pa = da - map->da + map->pa;
		dev_dbg(dev, "translate da 0x%llx to pa %pa\n", da, pa);
		return 0;
	}

	dev_err(dev, "Failed to translate da 0x%llx to pa\n", da);
	return -EINVAL;
}

static int sunxi_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct device *dev = rproc->dev.parent;
	void *va;

	dev_dbg(dev, "map memory: %pad+%zx\n", &mem->dma, mem->len);
	va = ioremap_wc(mem->dma, mem->len);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "Unable to map memory region: %pad+%zx\n",
			&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int sunxi_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	dev_dbg(rproc->dev.parent, "unmap memory: %pad\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static void *sunxi_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
#else
static void *sunxi_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len)
#endif
{
	struct device *dev = rproc->dev.parent;
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;
	phys_addr_t pa;
	int ret;
	size_t avail_len;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	/* first step: translate da to pa */
	ret = sunxi_rproc_da_to_pa(rproc, da, &pa);
	if (ret) {
		dev_err(dev, "invalid da 0x%llx\n", da);
		return NULL;
	}

	/* second step: get va from carveouts via pa */
	list_for_each_entry(carveout, &rproc->carveouts, node) {
		if ((pa >= carveout->dma) && (pa < carveout->dma + carveout->len)) {
			ptr = carveout->va + (pa - carveout->dma);
			avail_len = carveout->len - (pa - carveout->dma);
			if (avail_len < len) {
				dev_err(dev, "da(0x%llx) matched pa(0x%llx), "
					"but the length is not enough, "
					"need: 0x%lx, avail: 0x%lx\n",
					(unsigned long long)da, (unsigned long long)pa,
					(unsigned long)len, (unsigned long)avail_len);
				continue;
			}
			return ptr;
		}
	}
	return NULL;
}

#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
static int sunxi_rproc_mb_vq_rx_thread(void *data)
{
	struct sunxi_mbox *mb = (struct sunxi_mbox *)data;
	struct rproc *rproc = dev_get_drvdata(mb->client.dev);

	dev_dbg(&rproc->dev, "%s,%d\n", __func__, __LINE__);

	while (!kthread_should_stop()) {
		wait_event_interruptible(mb->rx_wait, rproc_vq_interrupt(rproc, mb->vq_id) != IRQ_NONE);
		/*
		 * We put the data receiving and processing part
		 * of the virtqueue in the bottom half.
		 */
		if (rproc_vq_interrupt(rproc, mb->vq_id) == IRQ_NONE)
			dev_dbg(&rproc->dev, "no message found in vq%d\n", mb->vq_id);

	}

	return 0;
}
#else
static void sunxi_rproc_mb_vq_work(struct work_struct *work)
{
	struct sunxi_mbox *mb = container_of(work, struct sunxi_mbox, vq_work);
	struct rproc *rproc = dev_get_drvdata(mb->client.dev);

	dev_dbg(&rproc->dev, "%s,%d\n", __func__, __LINE__);

	/*
	 * We put the data receiving and processing part
	 * of the virtqueue in the bottom half.
	 */
	if (rproc_vq_interrupt(rproc, mb->vq_id) == IRQ_NONE)
		dev_dbg(&rproc->dev, "no message found in vq%d\n", mb->vq_id);
}
#endif

static void sunxi_rproc_mb_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);
	struct sunxi_rproc *chip = rproc->priv;

	dev_dbg(&rproc->dev, "%s,%d name = arm-kick, vq_id = 0x%x\n", __func__, __LINE__, mb->vq_id);

	/*
	 * Data is sent from remote processor,
	 * which represents the virtqueue ID.
	 */
	mb->vq_id = *(u32 *)data;

#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	wake_up_interruptible(&chip->mb.rx_wait);
#else
	queue_work(chip->workqueue, &mb->vq_work);
#endif
}

static void sunxi_rproc_mb_tx_done(struct mbox_client *cl, void *msg, int r)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);

	dev_dbg(&rproc->dev, "%s,%d name = arm-kick, vq_id = 0x%x\n", __func__, __LINE__, mb->vq_id);
	devm_kfree(&rproc->dev, msg);
}

static int sunxi_rproc_request_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = &rproc->dev;
#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	struct sched_param sp = { .sched_priority = 1 };
#endif

	/* Initialise mailbox structure table */
	chip->mb.client.rx_callback = sunxi_rproc_mb_rx_callback;
	chip->mb.client.tx_done = sunxi_rproc_mb_tx_done;
	chip->mb.client.tx_block = false;
	chip->mb.client.dev = dev->parent;

	chip->mb.chan = mbox_request_channel_byname(&chip->mb.client, "arm-kick");
	if (IS_ERR(chip->mb.chan)) {
		dev_warn(dev, "cannot get arm-kick mbox, ret = %ld\n", PTR_ERR(chip->mb.chan));
		goto err_probe;
	}

#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	init_waitqueue_head(&chip->mb.rx_wait);
	chip->mb.rx_daemon = kthread_run(sunxi_rproc_mb_vq_rx_thread, &chip->mb, "mb_vq_rx_thread");
	WARN_ON_ONCE(sched_setscheduler_nocheck(chip->mb.rx_daemon, SCHED_FIFO, &sp) != 0);
#else
	INIT_WORK(&chip->mb.vq_work, sunxi_rproc_mb_vq_work);
#endif

	return 0;

err_probe:
	return PTR_ERR(chip->mb.chan);
}

static void sunxi_rproc_free_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;

#if IS_ENABLED(CONFIG_AW_RPMSG_RX_IN_KTHREAD)
	kthread_stop(chip->mb.rx_daemon);
	wake_up_interruptible(&chip->mb.rx_wait);
#endif

	mbox_free_channel(chip->mb.chan);
	chip->mb.chan = NULL;
}

static __maybe_unused int sunxi_request_firmware(struct rproc *rproc, const struct firmware **fw)
{
	struct device *dev = &rproc->dev;
	int ret;
	struct sunxi_rproc *chip = rproc->priv;
	struct device_node *np = chip->np;
	const char *name;
	u32 fw_size;
	int part_cnt, i;

	dev_dbg(dev, "try to load fw...\n");

	if (rproc->state == RPROC_DETACHED) {
		dev_dbg(dev, "try to load fw from memory...\n");
		ret = sunxi_request_firmware_from_memory(fw, rproc->firmware, dev);
		if (!ret) {
#ifdef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
			chip->is_zero_copy_mem_fw_active = 1;
			chip->zero_copy_mem_fw_name = kstrdup_const(rproc->firmware, GFP_KERNEL);
			if (!chip->zero_copy_mem_fw_name) {
				dev_err(dev, "load zero copy mem fw failed!");
				sunxi_unregister_memory_fw(rproc->firmware);
				chip->is_zero_copy_mem_fw_active = 0;
				((struct firmware *)(*fw))->data = NULL;
				release_firmware(*fw);
				*fw = NULL;
				return -ENOMEM;
			}
#endif
			dev_info(dev, "load fw from memory\n");
			/* when auto_boot inter firmware read from mem*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
			if (chip && !chip->is_using_kernel_fw) {
				chip->fw = *fw;
			}
#endif
			return 0;
		}
	}

	if (rproc->state == RPROC_DETACHED)
		dev_info(dev, "rproc has been booted by another entity(perhaps bootloader), "
			 "please confirm whether the ELF file is same!\n");

	dev_dbg(dev, "try to load fw from partitions...\n");
	part_cnt = of_property_count_strings(np, "fw-partitions");
	if (part_cnt > 0) {
		if (of_property_read_u32(np, "fw-partition-sectors", &fw_size)) {
			fw_size = 0x1000;
			dev_info(dev, "using default fw-partition-sectors: %u\n", fw_size);
		}
		dev_dbg(dev, "fw-partition-sectors: %u\n", fw_size);
		fw_size *= 512;
		for (i = 0; i < part_cnt; i++) {
			of_property_read_string_index(np, "fw-partitions", i, &name);
			ret = sunxi_request_firmware_from_partition(fw, name, fw_size, dev);
			if (!ret) {
				dev_info(dev, "load fw from partition %s\n", name);
				return 0;
			}
		}
	}

	ret = request_firmware(fw, rproc->firmware, dev);
	if (!ret) {
		dev_info(dev, "load fw from filesystem, filename: %s\n", rproc->firmware);
		return 0;
	}

	return ret;
}

static int suxni_rproc_elf_sanity_check(struct rproc *rproc, const struct firmware *fw)
{
	struct sunxi_rproc *chip = rproc->priv;

	if (!chip->is_using_kernel_fw) {
		struct device *dev = &rproc->dev;

		if (!chip->fw) {
			dev_info(dev, "using private firmware, skip checking here\n");
			return 0;
		}

		dev_info(dev, "checking private firmware\n");
		fw = chip->fw;
	}

	return rproc_elf_sanity_check(rproc, fw);
}

static int sunxi_rproc_unprepare(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;

	/* clean up carveout allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		if (entry->release)
			entry->release(rproc, entry);
		list_del(&entry->node);
		kfree(entry);
	}

	rproc_priv->pc_entry = 0;

	if (!chip->is_using_kernel_fw && chip->fw) {
#ifdef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
		if (chip->is_zero_copy_mem_fw_active) {
			sunxi_unregister_memory_fw(chip->zero_copy_mem_fw_name);
			kfree_const(chip->zero_copy_mem_fw_name);
			chip->zero_copy_mem_fw_name = NULL;
			chip->is_zero_copy_mem_fw_active = 0;

			((struct firmware *)chip->fw)->data = NULL;
		}
#endif
		release_firmware(chip->fw);
		chip->fw = NULL;
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static void sunxi_rproc_release_auto_boot_fw(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;

	if (!chip->is_using_kernel_fw && chip->fw) {
#ifdef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
		if (chip->is_zero_copy_mem_fw_active) {
			sunxi_unregister_memory_fw(chip->zero_copy_mem_fw_name);
			kfree_const(chip->zero_copy_mem_fw_name);
			chip->zero_copy_mem_fw_name = NULL;
			chip->is_zero_copy_mem_fw_active = 0;

			((struct firmware *)chip->fw)->data = NULL;
		}
#endif
		release_firmware(chip->fw);
		chip->fw = NULL;
	}
}
#else
static int sunxi_rproc_elf_find_section(struct rproc *rproc,
					 const struct firmware *fw,
					 const char *section_name,
					 const void **find_shdr);

static int sunxi_rproc_elf_get_fw_rsc_table(struct rproc *rproc,
	const struct firmware *fw,
	uint32_t *rsc_table_size, const uint8_t **rsc_table_data)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = chip->rproc_priv->dev;

	int ret;
	u8 class;
	u64 sh_addr;
	u64 offset_in_elf;
	const u8 *elf_data;
	const void *shdr;
	const char *section_name;

	section_name = ".resource_table";
	ret = sunxi_rproc_elf_find_section(rproc, fw, section_name, &shdr);
	if (ret) {
		dev_err(dev, "find section '%s' failed, ret: %d\n", section_name, ret);
		return -ENODEV;
	}

	elf_data = fw->data;
	class = fw_elf_get_class(fw);
	*rsc_table_size = elf_shdr_get_sh_size(class, shdr);
	offset_in_elf = elf_shdr_get_sh_offset(class, shdr);
	*rsc_table_data = elf_data + offset_in_elf;
	sh_addr = elf_shdr_get_sh_addr(class, shdr);

	dev_dbg(dev, "rsc table section, VMA: 0x%08llx, size: %u, data_addr: %px, fw_buf: %px\n",
		sh_addr, *rsc_table_size, *rsc_table_data, fw->data);

	return 0;
}

static int sunxi_release_private_firmware(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = chip->rproc_priv->dev;
	uint32_t rsc_table_size;
	const uint8_t *rsc_table_data;
	int ret;

	chip->fw_rsc_table_size = 0;
	ret = sunxi_rproc_elf_get_fw_rsc_table(rproc, chip->fw, &rsc_table_size, &rsc_table_data);
	if (!ret) {
		print_hex_dump_debug("saved rsc table: ", DUMP_PREFIX_OFFSET,
			16, 1, rsc_table_data, rsc_table_size, true);

		if (rsc_table_size <= RPROC_MAX_RSC_TABLE_SIZE) {
			chip->fw_rsc_table_size = rsc_table_size;
			memcpy(chip->fw_rsc_table_data, rsc_table_data, rsc_table_size);
		} else {
			dev_err(dev, "fw rsc table size(%u) is greater than %u!",
				rsc_table_size, RPROC_MAX_RSC_TABLE_SIZE);
		}
	} else {
		dev_err(dev, "get fw rsc table failed, ret: %d", ret);
	}

	release_firmware(chip->fw);
	chip->fw = NULL;
	return 0;
}
#endif

static int sunxi_rproc_start(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	int ret;

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_save(rproc_priv->share_irq);
#endif

	if (chip->is_booted)
		ret = sunxi_rproc_priv_attach(rproc_priv);
	else
		ret = sunxi_rproc_priv_start(rproc_priv);

	if (ret) {
		dev_err(rproc_priv->dev, "start remoteproc error\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_PM_SLEEP)
	ret = sunxi_rproc_standby_start(chip->rproc_standby);
	if (ret)
		dev_err(rproc_priv->dev, "start remoteproc standby error\n");
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	sunxi_rproc_release_auto_boot_fw(rproc);
#else
	if (!chip->is_using_kernel_fw && chip->fw) {
		/* release private firmware which is requested in prepare ops*/
		dev_dbg(rproc_priv->dev, "release private firmware in start ops\n");
		sunxi_release_private_firmware(rproc);
	}
#endif

	return 0;
}

static int sunxi_rproc_dump_exception_cause(struct sunxi_rproc *chip)
{
	struct fw_rsc_misc_rsc_data *rsc_data = chip->misc_rsc_data;

	if (!rsc_data) {
		dev_err(chip->rproc_priv->dev, "misc_rsc_data is NULL\n");
		return -EINVAL;
	}

	if (rsc_data->is_exception)
		dev_err(chip->rproc_priv->dev, "exception cause: %s\n", rsc_data->exception_cause);
	return 0;
}

#ifdef CONFIG_AW_RPROC_ENHANCED_TRACE_CRASH_DUMP
extern int sunxi_rproc_trace_dump(void *trace_mem, int trace_mem_len);
#endif

static int sunxi_rproc_stop(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	struct device *dev = rproc_priv->dev;
	int ret;

	if (rproc->state == RPROC_CRASHED)
		sunxi_rproc_dump_exception_cause(chip);

#ifdef CONFIG_AW_RPROC_ENHANCED_TRACE_CRASH_DUMP
	if (rproc->state == RPROC_CRASHED) {
		struct rproc_debug_trace *tmp, *trace;
		struct rproc_mem_entry *trace_mem;
		void *va;

		dev_info(rproc_priv->dev, "begin dump crash log before stop remoteproc!\n");

		list_for_each_entry_safe(trace, tmp, &rproc->traces, node) {
			trace_mem = &trace->trace_mem;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		va = rproc_da_to_va(rproc, trace_mem->da, trace_mem->len, NULL);
#else
		va = rproc_da_to_va(rproc, trace_mem->da, trace_mem->len);
#endif
			if (va) {
				dev_info(rproc_priv->dev, "trace mem: 0x%08x, len: %zu\n", trace_mem->da, trace_mem->len);
				sunxi_rproc_trace_dump(va, trace_mem->len);
			} else {
				dev_err(rproc_priv->dev, "failed to dump crash log, trace mem: 0x%08x, len: %zu\n", trace_mem->da, trace_mem->len);
			}
		}
	}
#endif

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_restore(rproc_priv->share_irq);
#endif

	ret = sunxi_rproc_priv_stop(rproc_priv);
	if (ret) {
		dev_err(dev, "stop remoteproc error\n");
		return ret;
	}

	chip->is_booted = 0;

#if IS_ENABLED(CONFIG_PM_SLEEP)
	ret = sunxi_rproc_standby_stop(chip->rproc_standby);
	if (ret)
		dev_err(rproc_priv->dev, "stop remoteproc standby error\n");
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	if ((rproc->state == RPROC_CRASHED) && !chip->is_using_kernel_fw && !chip->fw) {
		/* request private firmware because we have released it in start ops or attach ops */
		dev_dbg(dev, "request private firmware in stop ops\n");
		ret = sunxi_request_firmware(rproc, &chip->fw);
		if (!ret) {
			uint32_t rsc_table_size;
			const uint8_t *rsc_table_data;

			if (!chip->fw_rsc_table_size) {
				dev_warn(dev, "there is no saved rsc table in last private firmware!\n");
				goto exit_priv_fw_check;
			}

			ret = sunxi_rproc_elf_get_fw_rsc_table(rproc, chip->fw, &rsc_table_size, &rsc_table_data);
			if (!ret) {
				if (rsc_table_size == chip->fw_rsc_table_size) {
					ret = memcmp(rsc_table_data, chip->fw_rsc_table_data, rsc_table_size);
					if (ret) {
						dev_warn(dev, "the rsc table data is different with last private firmware!\n");
						print_hex_dump(KERN_WARNING, "current rsc table: ", DUMP_PREFIX_OFFSET,
							16, 1, rsc_table_data, rsc_table_size, true);
						print_hex_dump(KERN_WARNING, "last rsc table: ", DUMP_PREFIX_OFFSET,
							16, 1, chip->fw_rsc_table_data, chip->fw_rsc_table_size, true);
					}
				} else {
					dev_warn(dev, "the size of rsc table is different with last private firmware! "
						"current: %u, last: %u\n", rsc_table_size, chip->fw_rsc_table_size);
				}
			} else {
				dev_err(dev, "get fw rsc table failed, ret: %d\n", ret);
			}
		} else {
			dev_err(dev, "request private firmware failed: %d\n", ret);
		}
	}
exit_priv_fw_check:
#endif
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
/*
 * Attach to a running remote processors
 *
 * the remote processor is alread booted, so there is no need to do something to boot
 * the remoteproc processors. This callback is invoked only in fastboot mode
 */
static int sunxi_rproc_attach(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	struct device *dev = chip->rproc_priv->dev;
	int ret;

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	dev_info(dev, "remoteproc initialized in advanced fast boot mode\n");
#else
	dev_info(dev, "remoteproc initialized in general fast boot mode\n");
#endif

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_save(rproc_priv->share_irq);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	// init in sunxi_rproc_get_loaded_rsc_table
	if (!chip->rproc_priv->pc_entry) {
		dev_err(rproc_priv->dev, "pc_entry not init!\n");
		return -ENODEV;
	}
#endif

	ret = sunxi_rproc_priv_attach(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "attach remoteproc error\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_PM_SLEEP)
	ret = sunxi_rproc_standby_attach(chip->rproc_standby);
	if (ret)
		dev_err(rproc_priv->dev, "attach remoteproc standby error\n");
#endif

	if (!chip->is_using_kernel_fw && chip->fw) {
		/* release private firmware which is requested in prepare ops*/
		dev_dbg(rproc_priv->dev, "release private firmware in attach ops\n");
		sunxi_release_private_firmware(rproc);
	}

	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
/*
 * Detach from a running remote processors
 *
 * This rproc detach callback performs the opposite opreation to attach callback,
 * the remote core is not stopped and will be left to continue to run its booted firmware.
 * This callback is invoked only in fastboot mode
 */
static int sunxi_rproc_detach(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = chip->rproc_priv->dev;

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	dev_info(dev, "remoteproc deinitialized in advanced fast boot mode\n");
#else
	dev_info(dev, "remoteproc deinitialized in general fast boot mode\n");
#endif

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_restore(chip->rproc_priv->share_irq);
#endif

#if IS_ENABLED(CONFIG_PM_SLEEP)
	if (sunxi_rproc_standby_detach(chip->rproc_standby))
		dev_err(dev, "detach remoteproc standby error\n");
#endif

	return 0;
}
#endif

static int sunxi_rproc_prepare(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct sunxi_rproc *chip = rproc->priv;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem, *tmp;
	struct reserved_mem *rmem;
	int index = 0;
	int ret;
	u64 da;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	if (!chip->is_using_kernel_fw && !chip->fw && rproc->state != RPROC_DETACHED) {
#else
	if (!chip->is_using_kernel_fw && !chip->fw) {
#endif
		ret = sunxi_request_firmware(rproc, &chip->fw);
		if (ret < 0) {
			dev_err(dev, "request_firmware failed: %d\n", ret);
			return ret;
		}
		ret = rproc_elf_sanity_check(rproc, chip->fw);
		if (ret < 0) {
			dev_err(dev, "rproc_elf_sanity_check failed: %d\n", ret);
			release_firmware(chip->fw);
			chip->fw = NULL;
			return ret;
		}
	}


	/* Register associated reserved memory regions */
	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret) {
		dev_err(dev, "memory-region iterator init fail %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		ret = sunxi_rproc_pa_to_da(rproc, rmem->base, &da);
		if (ret) {
			dev_err(dev, "memory region not valid: %pa\n", &rmem->base);
			return -EINVAL;
		}

		/* No need to map vdev buffer */
		if (0 == strcmp(it.node->name, "vdev0buffer")) {
			mem = rproc_of_resm_mem_entry_init(dev, index,
							   rmem->size,
							   da,
							   it.node->name);
			/*
			 * The rproc_of_resm_mem_entry_init didn't save the
			 * physical address. Here we save it manually.
			 */
			if (mem)
				mem->dma = (dma_addr_t)rmem->base;
		} else {
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)rmem->base,
						   rmem->size, da,
						   NULL,
						   sunxi_rproc_mem_release,
						   it.node->name);
			if (mem)
				rproc_coredump_add_segment(rproc, da,
							   rmem->size);

			/* in attach mode, we need to early mapping mem */
			ret = sunxi_rproc_mem_alloc(rproc, mem);
			if (ret)
				goto clean_up_carveout;
		}

		if (!mem)
			return -ENOMEM;

		rproc_add_carveout(rproc, mem);
		index++;
	}

	return 0;
clean_up_carveout:
	/* clean up carveout allocations */
	list_for_each_entry_safe(mem, tmp, &rproc->carveouts, node) {
		if (mem->release)
			mem->release(rproc, mem);
		list_del(&mem->node);
		kfree(mem);
	}

	return -ENOMEM;
}



static int sunxi_rproc_parse_fw(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	struct elf32_hdr *ehdr;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	if (!chip->is_using_kernel_fw) {
		fw = chip->fw;
		ehdr = (struct elf32_hdr *)fw->data;
	} else {
		ehdr = (struct elf32_hdr *)fw->data;
	}

	if (!ehdr) {
		dev_err(dev, "%s,%d ehdr is null!\n", __func__, __LINE__);
		return -EINVAL;
	}

	chip->rproc_priv->pc_entry = ehdr->e_entry;

	/* if amp os only boot, skip load rsc table */
	if (chip->only_boot)
		return 0;

	/* check segment name, such as .resource_table */
	ret = rproc_elf_load_rsc_table(rproc, fw);
	if (ret) {
		rproc->cached_table = NULL;
		rproc->table_ptr = NULL;
		rproc->table_sz = 0;
		dev_warn(&rproc->dev, "no resource table found for this firmware\n");
	}

	return ret;
}

static void sunxi_rproc_kick(struct rproc *rproc, int vqid)
{
	struct sunxi_rproc *chip = rproc->priv;
	u32 *msg = NULL;
	int err;

	dev_dbg(&rproc->dev, "%s,%d vqid = 0x%x\n", __func__, __LINE__, vqid);

	if (WARN_ON(vqid >= MBOX_NB_VQ))
		return;

	/*
	 * Because of the implementation of sunxi msgbox(mailbox controller),
	 * the type of mailbox message should be u32.
	 */
	msg = devm_kzalloc(&rproc->dev, sizeof(*msg), GFP_KERNEL);
	if (!msg)
		return;

	*msg = vqid;

	/* Remeber to free msg in mailbox tx_done callback */
	err = mbox_send_message(chip->mb.chan, (void *)msg);
	if (err < 0) {
		devm_kfree(&rproc->dev, msg);
		dev_err(&rproc->dev, "%s,%d kick err:%d\n",
			__func__, __LINE__, err);
	}
	return;
}

static int sunxi_rproc_elf_find_section(struct rproc *rproc,
					 const struct firmware *fw,
					 const char *section_name,
					 const void **find_shdr)
{
	struct device *dev = &rproc->dev;
	const void *shdr, *name_table_shdr;
	int i;
	const char *name_table;
	const u8 *elf_data = (void *)fw->data;
	u8 class = fw_elf_get_class(fw);
	const void *ehdr = elf_data;
	u16 shnum = elf_hdr_get_e_shnum(class, ehdr);
	u32 elf_shdr_get_size = elf_size_of_shdr(class);
	u16 shstrndx = elf_hdr_get_e_shstrndx(class, ehdr);

	shdr = elf_data + elf_hdr_get_e_shoff(class, ehdr);
	name_table_shdr = shdr + (shstrndx * elf_shdr_get_size);
	name_table = elf_data + elf_shdr_get_sh_offset(class, name_table_shdr);

	for (i = 0; i < shnum; i++, shdr += elf_shdr_get_size) {
		u64 size = elf_shdr_get_sh_size(class, shdr);
		u32 name = elf_shdr_get_sh_name(class, shdr);

		if (strcmp(name_table + name, section_name))
			continue;

		*find_shdr = shdr;
		dev_dbg(dev, "%s,%d %s addr 0x%llx, size 0x%llx\n",
			__func__, __LINE__, section_name, elf_shdr_get_sh_addr(class, shdr), size);

		return 0;
	}

	return -EINVAL;

}

static int sunxi_rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc *chip = rproc->priv;
	const void *ehdr;
	const void *phdr;
	const void *shdr;
	void *ptr;
	int i, ret = 0;
	const u8 *elf_data;
	u8 class;
	u64 offset, da, memsz, filesz;
	u32 type;
	u16 phnum;
	u32 elf_phdr_get_size;

	if (!chip->is_using_kernel_fw) {
		fw = chip->fw;
		if (!fw) {
			dev_err(dev, "private firmware not exist! perhaps request it failed in stop ops when rproc is crashed");
			return -EINVAL;
		}
		elf_data = fw->data;
	} else
		elf_data = fw->data;

	class = fw_elf_get_class(fw);
	elf_phdr_get_size = elf_size_of_phdr(class);
	ehdr = elf_data;
	phdr = elf_data + elf_hdr_get_e_phoff(class, ehdr);
	phnum = elf_hdr_get_e_phnum(class, ehdr);

	/* get version from elf  */
	ret = sunxi_rproc_elf_find_section(rproc, fw, ".version_table", &shdr);
	if (ret) {
		dev_warn(dev, "%s,%d find segments err\n", __func__, __LINE__);
		/* Lack of ".version_table" should not be assumed as an error */
		ret = 0;
	} else {
		dev_info(dev, "firmware version: %s\n", elf_data + elf_shdr_get_sh_offset(class, shdr));
	}

	/* we must copy .resource_table, when use simulator to debug */
	if (simulator_debug) {
		dev_dbg(dev, "%s,%d only load .resource_table data\n",
				__func__, __LINE__);

		ret = sunxi_rproc_elf_find_section(rproc, fw, ".resource_table", &shdr);
		if (ret) {
			dev_err(dev, "%s,%d find segments err\n", __func__, __LINE__);
			return ret;
		}

		da = elf_shdr_get_sh_addr(class, shdr);
		memsz = elf_shdr_get_sh_size(class, shdr);
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
#else
		ptr = rproc_da_to_va(rproc, da, memsz);
#endif
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%llx mem 0x%llx\n", da, memsz);
			return -EINVAL;
		}
		memcpy(ptr, elf_data + elf_shdr_get_sh_offset(class, shdr), memsz);

		return 0;
	}

	/* arm can write/read local ram */
	sunxi_rproc_priv_set_localram(chip->rproc_priv, 1);

	dev_err(dev, "%s,%d, %d\n", __func__, __LINE__, phnum);

	/* go through the available ELF segments */
	for (i = 0; i < phnum; i++, phdr += elf_phdr_get_size) {
		da = elf_phdr_get_p_paddr(class, phdr);
		memsz = elf_phdr_get_p_memsz(class, phdr);
		filesz = elf_phdr_get_p_filesz(class, phdr);
		offset = elf_phdr_get_p_offset(class, phdr);
		type = elf_phdr_get_p_type(class, phdr);

		if (type != PT_LOAD)
			continue;

		dev_dbg(dev, "phdr: type %d da 0x%llx memsz 0x%llx filesz 0x%llx\n",
			type, da, memsz, filesz);

		if ((memsz == 0) || (filesz == 0))
			continue;

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%llx memsz 0x%llx\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%llx avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 15, 0)
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
#else
		ptr = rproc_da_to_va(rproc, da, memsz);
#endif
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%llx mem 0x%llx\n", da,
				memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (filesz)
			memcpy(ptr, elf_data + offset, filesz);

		/*
		 * Zero out remaining memory for this segment.
		 *
		 * This isn't strictly required since dma_alloc_coherent already
		 * did this for us. albeit harmless, we may consider removing
		 * this.
		 */
		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);
	}

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static struct resource_table *sunxi_rproc_get_loaded_rsc_table(struct rproc *rproc, size_t *table_sz)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = rproc->dev.parent;
	const struct firmware *firmware_p;
	const void *shdr;
	u8 class;
	u64 sh_addr;
	int ret;

	dev_dbg(dev, "get_loaded_rsc_table %s\n", rproc->firmware);

	if (!chip->is_using_kernel_fw) {
		firmware_p = chip->fw;
	} else {
		ret = sunxi_request_firmware(rproc, &firmware_p);
		if (ret < 0) {
			dev_err(dev, "request_firmware failed: %d\n", ret);
			return ERR_PTR(-ENODEV);
			//return ERR_PTR(-EPROBE_DEFER);
		}
		ret = rproc_elf_sanity_check(rproc, firmware_p);
		if (ret < 0) {
			dev_err(dev, "rproc_elf_sanity_check failed: %d\n", ret);
			release_firmware(firmware_p);
			return ERR_PTR(-EINVAL);
		}
	}

	ret = sunxi_rproc_elf_find_section(rproc, firmware_p,
							 ".resource_table", &shdr);
	if (ret) {
		dev_err(dev, "%s,%d find segments err\n", __func__, __LINE__);
		if (chip->is_using_kernel_fw) {
			release_firmware(firmware_p);
		}
		return ERR_PTR(-ENODEV);
	}

	if (rproc->state == RPROC_DETACHED) {
		chip->rproc_priv->pc_entry = rproc_elf_get_boot_addr(rproc, firmware_p);
	}

	class = fw_elf_get_class(firmware_p);
	*table_sz = elf_shdr_get_sh_size(class, shdr);
	sh_addr = elf_shdr_get_sh_addr(class, shdr);

	chip->rsc_table_va = rproc_da_to_va(rproc, sh_addr, *table_sz, NULL);

	if (IS_ERR_OR_NULL(chip->rsc_table_va)) {
		dev_err(dev, "Unable to find loaded rsc table: %s\n", rproc->firmware);
		chip->rsc_table_va = NULL;
		if (chip->is_using_kernel_fw) {
			release_firmware(firmware_p);
		}

		return ERR_PTR(-ENOMEM);
	}

	dev_info(dev, "loaded resource table, addr: 0x%08llx, size: %zu\n", sh_addr, *table_sz);

	if (chip->is_using_kernel_fw) {
		release_firmware(firmware_p);
	}

	return (struct resource_table *)chip->rsc_table_va;
}
#endif

static u64 suxni_rproc_elf_get_boot_addr(struct rproc *rproc, const struct firmware *fw)
{
	u64 data = 0;
	struct sunxi_rproc *chip = rproc->priv;

	if (!chip->is_using_kernel_fw) {
		fw = chip->fw;
	}

	data = rproc_elf_get_boot_addr(rproc, fw);
	dev_dbg(&rproc->dev, "%s,%d elf boot addr = 0x%llx\n", __func__, __LINE__, data);
	return data;
}

struct resource_table *suxni_rproc_elf_find_loaded_rsc_table(struct rproc *rproc,
							     const struct firmware *fw)
{
	struct sunxi_rproc *chip = rproc->priv;

	if (!chip->is_using_kernel_fw) {
		fw = chip->fw;
	}

	return rproc_elf_find_loaded_rsc_table(rproc, fw);
}

#if IS_ENABLED(CONFIG_AW_RPROC_ENHANCED_TRACE)
extern struct dentry *sunxi_rproc_create_aw_trace_file(const char *name, struct rproc *rproc,
				       struct rproc_debug_trace *trace);
#endif

static int sunxi_rproc_handle_aw_trace(struct rproc *rproc, void *ptr,
			       int offset, int avail)
{
	struct fw_rsc_aw_trace *rsc = ptr;
	struct rproc_debug_trace *trace;
	struct device *dev = rproc->dev.parent;
	char name[64];

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "trace rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "trace rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	trace = kzalloc(sizeof(*trace), GFP_KERNEL);
	if (!trace)
		return -ENOMEM;

	/* set the trace buffer dma properties */
	trace->trace_mem.len = rsc->len;
	trace->trace_mem.da = rsc->da;

	/* set pointer on rproc device */
	trace->rproc = rproc;

	/* make sure snprintf always null terminates, even if truncating */
	snprintf(name, sizeof(name), "aw_trace_%s", rsc->name);

	/* create the debugfs entry */
#if IS_ENABLED(CONFIG_AW_RPROC_ENHANCED_TRACE)
	trace->tfile = sunxi_rproc_create_aw_trace_file(name, rproc, trace);
	if (!trace->tfile) {
		kfree(trace);
		return -EINVAL;
	}
#endif

	list_add_tail(&trace->node, &rproc->traces);

	rproc->num_traces++;

	dev_info(dev, "add trace mem '%s', da: 0x%08x, len: %u\n", name, rsc->da, rsc->len);
	return 0;
}

static int sunxi_rproc_handle_aw_misc_rsc(struct rproc *rproc, void *ptr,
			       int offset, int avail)
{
	void *va;
	struct fw_rsc_misc_rsc_data *rsc_data = NULL;
	struct fw_rsc_misc_resource *rsc = ptr;
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;

	if (sizeof(*rsc) > avail) {
		dev_err(dev, "rsc is truncated\n");
		return -EINVAL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	va = rproc_da_to_va(rproc, rsc->da, rsc->len, NULL);
#else
	va = rproc_da_to_va(rproc, rsc->da, rsc->len);
#endif
	rsc_data = (struct fw_rsc_misc_rsc_data *)va;
	chip->misc_rsc_data = rsc_data;
	dev_info(dev, "save misc_rsc_data\n");
	return 0;
}

static int sunxi_rproc_handle_rsc(struct rproc *rproc, u32 rsc_type,
			       void *ptr, int offset, int avail)
{
#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	int ret;
	struct sunxi_rproc *chip = rproc->priv;
#endif

	dev_info(rproc->dev.parent, "handle vendor resource, type: %u\n", rsc_type);
	if (rsc_type == RSC_TYPE_AW_TRACE)
		return sunxi_rproc_handle_aw_trace(rproc, ptr, offset, avail);

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	ret = sunxi_rproc_subdev_handle_rsc(&chip->subdev, rsc_type, ptr, offset, avail);
	if (ret == RSC_HANDLED)
		return ret;
#endif

	if (rsc_type == RSC_TYPE_AW_MISC_RSC)
		return sunxi_rproc_handle_aw_misc_rsc(rproc, ptr, offset, avail);

	dev_info(&rproc->dev, "%s nobody handle RSC_VENDOR\n", __func__);
	return RSC_IGNORED;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
static int suxni_rproc_trigger_auto_boot(struct rproc *rproc)
{
	if (rproc->auto_boot)
		rproc_boot(rproc);

	return 0;
}

int sunxi_rproc_request_firmware(struct rproc *rproc, const struct firmware **fw)
{
	int ret;
	struct sunxi_rproc *chip = rproc->priv;

	dev_dbg(&rproc->dev, "%s,%d\n", __func__, __LINE__);

	ret = sunxi_request_firmware(rproc, fw);

	if (chip && !chip->is_using_kernel_fw) {
		chip->fw = *fw;
	}

	return ret;
}
#endif
static struct rproc_ops sunxi_rproc_ops = {
	.prepare	= sunxi_rproc_prepare,
	.unprepare	= sunxi_rproc_unprepare,
	.start		= sunxi_rproc_start,
	.stop		= sunxi_rproc_stop,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	.attach		= sunxi_rproc_attach,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	.detach		= sunxi_rproc_detach,
#endif
	.da_to_va	= sunxi_rproc_da_to_va,
	.kick		= sunxi_rproc_kick,
	.parse_fw	= sunxi_rproc_parse_fw,
	.find_loaded_rsc_table = suxni_rproc_elf_find_loaded_rsc_table,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	.get_loaded_rsc_table = sunxi_rproc_get_loaded_rsc_table,
#endif
	.load		= sunxi_rproc_elf_load_segments,
	.get_boot_addr	= suxni_rproc_elf_get_boot_addr,
	.sanity_check	= suxni_rproc_elf_sanity_check,
	.handle_rsc = sunxi_rproc_handle_rsc,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	.request_firmware = sunxi_rproc_request_firmware,
	.trigger_auto_boot = suxni_rproc_trigger_auto_boot,
#endif
};

static const struct of_device_id sunxi_rproc_match[] = {
	{ .compatible = "allwinner,hifi4-rproc", .data = "hifi4" },
	{ .compatible = "allwinner,c906-rproc", .data = "c906" },
	{ .compatible = "allwinner,e906-rproc", .data = "e906" },
	{ .compatible = "allwinner,e907-rproc", .data = "e907" },
	{ .compatible = "allwinner,arm-rtos-rproc", .data = "arm_rtos" },
	{ .compatible = "allwinner,arm-barematal-rproc", .data = "arm_baremetal" },

	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_rproc_match);

static int sunxi_rproc_resource_get(struct rproc *rproc, struct platform_device *pdev)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *chip = rproc->priv;
	int ret;

	chip->rproc_priv = sunxi_rproc_priv_find(chip->name);
	if (!chip->rproc_priv) {
		dev_err(dev, "find rproc priv error\n");
		return -EINVAL;
	}

	ret = sunxi_rproc_priv_resource_get(chip->rproc_priv, pdev);
	if (ret) {
		dev_err(dev, "resource get error\n");
		return ret;
	}

	chip->rproc_priv->rproc = rproc;

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	ret = of_property_read_string(dev->of_node, "share-irq", &chip->rproc_priv->share_irq);
	if (ret < 0) {
		chip->rproc_priv->share_irq = NULL;
		dev_warn(dev, "get AMP share-irq cfg failed, ret: %d\n", ret);
	}
#endif

	chip->rproc_standby = sunxi_rproc_standby_find(chip->name);
	if (!chip->rproc_standby) {
		dev_warn(dev, "find rproc standby error\n");
	} else {
		chip->rproc_standby->rproc_priv = chip->rproc_priv;
		ret = sunxi_rproc_standby_init(chip->rproc_standby, pdev);
		if (ret) {
			dev_err(dev, "standby init failed, return: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static void sunxi_rproc_resource_put(struct rproc *rproc, struct platform_device *pdev)
{
	struct sunxi_rproc *chip = rproc->priv;
	sunxi_rproc_priv_resource_put(chip->rproc_priv, pdev);

	sunxi_rproc_standby_exit(chip->rproc_standby, pdev);
}

int sunxi_rproc_report_crash(const char *name, enum rproc_crash_type type)
{
	struct sunxi_rproc *chip, *tmp;

	list_for_each_entry_safe(chip, tmp, &sunxi_rproc_list, list) {
		/* report is noneed, set state detached by master */
		if (chip->rproc->state == RPROC_DETACHED)
			continue;
		if (!strcmp(chip->rproc->name, name)) {
			rproc_report_crash(chip->rproc, type);
			return 0;
		}
	}

	return -ENXIO;
}
EXPORT_SYMBOL(sunxi_rproc_report_crash);

static int sunxi_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct sunxi_rproc *chip;
	struct device_node *np = dev->of_node;
	struct rproc *rproc;
	const char *fw_name = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	struct resource_table *loaded_table;
#endif
	int ret;
	u8 rproc_elf_class = ELFCLASS32;

	dev_info(dev, "sunxi rproc driver %s\n", SUNXI_RPROC_VERSION);

	of_id = of_match_device(sunxi_rproc_match, dev);
	if (!of_id) {
		dev_err(dev, "No device of_id found\n");
		ret = -EINVAL;
		goto err_out;
	}

	/* we need to read firmware name at first. */
	ret = of_property_read_string(np, "firmware-name", &fw_name);
	if (ret < 0) {
		dev_info(dev, "failed to get firmware-name\n");
		fw_name = NULL;
	}

	rproc = rproc_alloc(dev, np->name, &sunxi_rproc_ops, fw_name, sizeof(*chip));
	if (!rproc) {
		ret = -ENOMEM;
		goto err_out;
	}

	rproc->has_iommu = false;
	rproc->auto_boot = of_property_read_bool(np, "auto-boot");
	rproc->recovery_disabled = of_property_read_bool(np, "recovery_disabled");

	if (of_property_read_bool(np, RPROC_COREDUMP_PROPERTY_NAME))
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
		rproc->dump_conf = RPROC_COREDUMP_ENABLED;
#else
	{
#endif

	if (!strcmp((const char *)of_id->data, "c906")) {
		rproc_elf_class = ELFCLASS64;
	}
	rproc_coredump_set_elf_info(rproc, rproc_elf_class, EM_NONE);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	}
#endif

	chip = rproc->priv;
	chip->rproc = rproc;
	chip->name = (char *)of_id->data;
	chip->np = np;
	chip->is_using_kernel_fw = of_property_read_bool(np, USING_KERNEL_FW_PROPERTY_NAME);
	chip->skip_shutdown = of_property_read_bool(np, "skip-shutdown");
	chip->is_nmi_available = of_property_read_bool(np, RPROC_NMI_AVAILABLE_PROPERTY_NAME);
	if (chip->is_nmi_available) {
		chip->wait_nmi_complete_timeout = RPROC_DEFAULT_WAIT_NMI_COMPLETE_TIMEOUT;
		of_property_read_u32(np, RPROC_WAIT_NMI_COMPLETE_TIMEOUT_PROPERTY_NAME, &chip->wait_nmi_complete_timeout);
	}
	chip->only_boot = of_property_read_bool(np, "only-boot");

	ret = sunxi_rproc_resource_get(rproc, pdev);
	if (ret) {
		dev_err(dev, "Failed to get resource\n");
		goto free_rproc;
	}
	chip->rproc_priv->np = np;

	chip->is_booted = sunxi_rproc_priv_is_booted(chip->rproc_priv);
	if (chip->is_booted) {
		rproc->state = RPROC_DETACHED;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
		atomic_inc(&rproc->power);
#endif
	}

	chip->workqueue = create_workqueue(dev_name(dev));
	if (!chip->workqueue) {
		dev_err(dev, "Cannot create workqueue\n");
		ret = -ENOMEM;
		goto put_resource;
	}

	platform_set_drvdata(pdev, rproc);

	ret = sunxi_rproc_request_mbox(rproc);
	if (ret) {
		dev_err(dev, "Request mbox failed\n");
		goto destroy_workqueue;
	}

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	chip->subdev.io_base = chip->rproc_priv->io_base;
	sunxi_rproc_add_subdev(&chip->subdev, rproc, pdev);
#endif

	if (rproc->state == RPROC_DETACHED) {
		int is_mem_fw_reg_success = 0;
		struct device_node *fw_np = of_parse_phandle(np, AW_MEM_FW_REGION_PROPERTY_NAME, 0);
		if (fw_np) {
			struct resource r;
			resource_size_t len;

			ret = of_address_to_resource(fw_np, 0, &r);
			if (!ret) {
				len = resource_size(&r);

				dev_info(dev, "register memory firmware('%s') for '%s', addr: 0x%llx, size: %ld\n",
					rproc->firmware, chip->name, (long long)r.start, (long)len);
				ret = sunxi_register_memory_fw(rproc->firmware, r.start, len);
				if (ret < 0) {
					dev_err(dev, "register memory firmware('%s') failed. ret: %d\n", rproc->firmware, ret);
				} else {
					is_mem_fw_reg_success = 1;
				}
			} else {
				dev_err(dev, "parse dt node '%s' failed, ret: %d\n", fw_np->full_name, ret);
			}
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
			if (!chip->only_boot) {
				ret = sunxi_rproc_prepare(rproc);
				if (ret)
					dev_err(dev, "prepare firmware%s fail ret: %d\n", fw_np->full_name, ret);

				sunxi_rproc_parse_fw(rproc, chip->fw);
				if (ret)
					dev_err(dev, "parse firmware%s fail ret: %d\n", fw_np->full_name, ret);

				loaded_table = suxni_rproc_elf_find_loaded_rsc_table(rproc, chip->fw);
				if (loaded_table) {
					memcpy(loaded_table, rproc->cached_table, rproc->table_sz);
					rproc->table_ptr = loaded_table;
				}
				rproc->cached_table = NULL;
			}
#endif
		} else {
			dev_warn(dev, "property '%s' for memory firmware not exist when remoteproc has been booted up by bootloader!\n",
				AW_MEM_FW_REGION_PROPERTY_NAME);
		}

		if (!is_mem_fw_reg_success && rproc->auto_boot) {
			dev_warn(dev, "Register memory firmware failed when auto-boot is enabled!\n");
			rproc->auto_boot = false;
		}
	} else {
		/* If the booloader doesn't boot up remoteproc(eg: firmware verify failed or other reason),
		 * we need to set auto_boot to false. Otherwise the kernel will crash(non fast boot mode),
		 * or the remoteproc will probe failed(fast boot mode). Since some module has not been initialized
		 * in non fast boot mode. It is a abnormal situation whatever the system is in non fast boot mode
		 * or fast boot mode(remoteproc module and other related module init in early stage and
		 * set auto-boot property in dts).
		 */
		if (rproc->auto_boot) {
			dev_warn(dev, "The bootloader doesn't boot up the remoteproc when auto-boot is enabled!\n");
			rproc->auto_boot = false;
		}
	}

	dev_info(dev, "is_using_kernel_fw: %d", chip->is_using_kernel_fw);
	if (chip->is_nmi_available)
		dev_info(dev, "wait_nmi_complete_timeout: %ums", chip->wait_nmi_complete_timeout);

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "Failed to register rproc\n");
		goto free_mbox;
	}

	list_add(&chip->list, &sunxi_rproc_list);

	if (chip->only_boot && chip->is_booted)
		rproc->state = RPROC_RUNNING;

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	sunxi_rproc_subdev_set_parent(&chip->subdev);
#endif

	dev_info(dev, "sunxi rproc driver probe ok\n");

	return ret;

free_mbox:
	sunxi_rproc_free_mbox(rproc);
destroy_workqueue:
	destroy_workqueue(chip->workqueue);
put_resource:
	sunxi_rproc_resource_put(rproc, pdev);
free_rproc:
	rproc_free(rproc);
err_out:
	return ret;
}

int sunxi_rproc_send_mbox_msg(struct rproc *rproc, u32 data)
{
	struct sunxi_rproc *chip;
	u32 *msg = NULL;
	int err;

	if ((rproc == NULL) || (rproc->priv == NULL))
		return -EINVAL;

	dev_dbg(&rproc->dev, "%s,%d send mbox msg(0x%x)\n", __func__, __LINE__, data);

	chip = rproc->priv;

	/*
	 * Because of the implementation of sunxi msgbox(mailbox controller),
	 * the type of mailbox message should be u32.
	 */
	msg = devm_kzalloc(&rproc->dev, sizeof(*msg), GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	*msg = data;

	/* Remeber to free msg in mailbox tx_done callback */
	err = mbox_send_message(chip->mb.chan, (void *)msg);
	if (err < 0) {
		devm_kfree(&rproc->dev, msg);
		dev_err(&rproc->dev, "%s,%d send mbox msg err:%d\n",
			__func__, __LINE__, err);
	}
	return 0;
}

struct sunxi_rproc_standby *sunxi_rproc_get_standby_handler(struct sunxi_rproc *chip)
{
	struct sunxi_rproc_standby *rproc_standby = NULL;

	if (chip && chip->rproc_standby)
		rproc_standby = chip->rproc_standby;

	return rproc_standby;
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
static int sunxi_rproc_pm_prepare(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_standby *rproc_standby = ddata->rproc_standby;

	if (!rproc_standby || !rproc_standby->ctrl_en)
		return 0;

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV_STANDBY)
	rproc_standby->stdby_dev = ddata->subdev.stdby_dev;
#endif

	return sunxi_rproc_standby_prepare(rproc_standby);
}

static int sunxi_rproc_suspend(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_standby *rproc_standby = ddata->rproc_standby;

	if (!rproc_standby || !rproc_standby->ctrl_en)
		return 0;

	return sunxi_rproc_standby_suspend(rproc_standby);
}

static int sunxi_rproc_resume(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_standby *rproc_standby = ddata->rproc_standby;

	if (!rproc_standby || !rproc_standby->ctrl_en)
		return 0;

	return sunxi_rproc_standby_resume(rproc_standby);
}

static void sunxi_rproc_pm_complete(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_standby *rproc_standby = ddata->rproc_standby;

	if (!rproc_standby || !rproc_standby->ctrl_en)
		return;

	sunxi_rproc_standby_complete(rproc_standby);
}

static struct dev_pm_ops sunxi_rproc_pm_ops = {
	.prepare = sunxi_rproc_pm_prepare,
	.suspend = sunxi_rproc_suspend,
	.resume = sunxi_rproc_resume,
	.complete = sunxi_rproc_pm_complete,
};
#endif

static int sunxi_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *chip = rproc->priv;

	if (atomic_read(&rproc->power) > 0)
		rproc_shutdown(rproc);

#if IS_ENABLED(CONFIG_AW_RPROC_SUBDEV)
	sunxi_rproc_remove_subdev(&chip->subdev);
#endif

	list_del(&chip->list);

	if (!list_empty(&sunxi_rproc_list))
		list_del(&sunxi_rproc_list);

	rproc_del(rproc);

	sunxi_rproc_free_mbox(rproc);

	destroy_workqueue(chip->workqueue);

	sunxi_rproc_resource_put(rproc, pdev);

	rproc_free(rproc);

	return 0;
}

static void sunxi_rproc_shutdown(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *chip = rproc->priv;

	if (chip->skip_shutdown) {
		if (chip->rproc_priv)
			dev_info(chip->rproc_priv->dev, "skip shutdown\n");
		return;
	}

	if (sunxi_rproc_priv_is_booted(chip->rproc_priv))
		rproc_shutdown(rproc);

	return ;
}

static struct platform_driver sunxi_rproc_driver = {
	.probe = sunxi_rproc_probe,
	.remove = sunxi_rproc_remove,
	.shutdown = sunxi_rproc_shutdown,
	.driver = {
		.name = "sunxi-rproc", /* dev name */
		.of_match_table = sunxi_rproc_match,
#if IS_ENABLED(CONFIG_PM_SLEEP)
		.pm = &sunxi_rproc_pm_ops,
#endif
	},
};
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
static int __init sunxi_rproc_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_rproc_driver);

	return ret;
}

static void __exit sunxi_rproc_exit(void)
{
	platform_driver_unregister(&sunxi_rproc_driver);
}

core_initcall(sunxi_rproc_init);
module_exit(sunxi_rproc_exit);
#else
module_platform_driver(sunxi_rproc_driver);
#endif

MODULE_DESCRIPTION("Allwinnertech Remote Processor Control Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPROC_VERSION);

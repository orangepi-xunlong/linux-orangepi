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
#include <linux/clk.h>
#include "remoteproc_internal.h"
#include "sunxi_rproc_boot.h"
#include "sunxi_rproc_standby.h"
#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
#include "sunxi_share_interrupt/sunxi_share_interrupt.h"
#endif

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
#include "sunxi_rproc_firmware.h"
#endif

#include <remoteproc/sunxi_remoteproc.h>

#define SUNXI_RPROC_VERSION "2.3.2"

#define MBOX_NB_VQ		2

static LIST_HEAD(sunxi_rproc_list);
struct sunxi_mbox {
	struct mbox_chan *chan;
	struct mbox_client client;
	struct work_struct vq_work;
	int vq_id;
};

struct sunxi_rproc {
	struct sunxi_rproc_priv *rproc_priv;  /* dsp/riscv private resources */
#if IS_ENABLED(CONFIG_PM_SLEEP)
	struct sunxi_rproc_standby *rproc_standby;
#endif
	struct sunxi_mbox mb;
	struct workqueue_struct *workqueue;
	struct list_head list;
	struct rproc *rproc;
	void __iomem *rsc_table_va;
	bool is_booted;
	char *name;
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

static void *sunxi_rproc_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct device *dev = rproc->dev.parent;
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;
	phys_addr_t pa;
	int ret;

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
			return ptr;
		}
	}
	return NULL;
}

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

	queue_work(chip->workqueue, &mb->vq_work);
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

	/* Initialise mailbox structure table */
	chip->mb.client.rx_callback = sunxi_rproc_mb_rx_callback;
	chip->mb.client.tx_done = sunxi_rproc_mb_tx_done;
	chip->mb.client.tx_block = false;
	chip->mb.client.dev = dev->parent;

	chip->mb.chan = mbox_request_channel_byname(&chip->mb.client, "arm-kick");
	if (IS_ERR(chip->mb.chan)) {
		if (PTR_ERR(chip->mb.chan) == -EPROBE_DEFER)
			goto err_probe;
		dev_warn(dev, "cannot get arm-kick mbox\n");
		chip->mb.chan = NULL;
	}

	INIT_WORK(&chip->mb.vq_work, sunxi_rproc_mb_vq_work);

	return 0;

err_probe:
	mbox_free_channel(chip->mb.chan);
	return -EPROBE_DEFER;
}

static void sunxi_rproc_free_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;

	mbox_free_channel(chip->mb.chan);
	chip->mb.chan = NULL;
}

static int sunxi_rproc_unprepare(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;

	/* clean up carveout allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		if (entry->release)
			entry->release(rproc, entry);
		list_del(&entry->node);
		kfree(entry);
	}

	return 0;
}

static int sunxi_rproc_start(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	int ret;

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_save(rproc_priv->share_irq);
#endif

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

	return 0;
}
#ifdef CONFIG_AW_RPROC_ENHANCED_TRACE
extern int sunxi_rproc_trace_dump(void *trace_mem, int trace_mem_len);
#endif

static int sunxi_rproc_stop(struct rproc *rproc)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct sunxi_rproc_priv *rproc_priv = chip->rproc_priv;
	int ret;

#ifdef CONFIG_AW_RPROC_ENHANCED_TRACE
	if (rproc->state == RPROC_CRASHED) {
		struct rproc_debug_trace *tmp, *trace;
		struct rproc_mem_entry *trace_mem;
		void *va;

		dev_info(rproc_priv->dev, "begin dump crash log before stop remoteproc!\n");

		list_for_each_entry_safe(trace, tmp, &rproc->traces, node) {
			trace_mem = &trace->trace_mem;
			va = rproc_da_to_va(rproc, trace_mem->da, trace_mem->len, NULL);
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
		dev_err(rproc_priv->dev, "stop remoteproc error\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_PM_SLEEP)
	ret = sunxi_rproc_standby_stop(chip->rproc_standby);
	if (ret)
		dev_err(rproc_priv->dev, "stop remoteproc standby error\n");
#endif

	return 0;
}

#ifndef CONFIG_AW_RPROC_FAST_BOOT
/*
 * Attach to get firmware entry
 *
 * if start riscv in uboot, attach process lack initialization of riscv start address
 * as a result, when riscv panic, linux cannot restart riscv
 * so, need to obtain the riscv entry in the attach process
 */
static int sunxi_rproc_attach_get_entry(struct rproc *rproc)
{
	const struct firmware *fw;
	struct device *dev;
	struct sunxi_rproc *chip;
	struct elf32_hdr *ehdr;
	int ret;

	dev = &rproc->dev;
	chip = rproc->priv;
	ret = request_firmware(&fw, rproc->firmware, dev);
	if (ret < 0) {
		dev_err(dev, "attach request_firmware failed: %d\n", ret);
		return ret;
	}

	ehdr  = (struct elf32_hdr *)fw->data;
	chip->rproc_priv->pc_entry = ehdr->e_entry;
	release_firmware(fw);

	return 0;
}
#endif

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

	dev_info(dev, "remoteproc initialized in fast boot mode\n");

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_save(rproc_priv->share_irq);
#endif

#ifndef CONFIG_AW_RPROC_FAST_BOOT
	ret = sunxi_rproc_attach_get_entry(rproc);
	if (ret) {
		dev_err(rproc_priv->dev, "attach get entry error\n");
		return ret;
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

	return 0;
}

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

	dev_info(dev, "remoteproc deinitialized in fast boot mode\n");

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	sunxi_arch_interrupt_restore(chip->rproc_priv->share_irq);
#endif

#if IS_ENABLED(CONFIG_PM_SLEEP)
	if (sunxi_rproc_standby_detach(chip->rproc_standby))
		dev_err(dev, "detach remoteproc standby error\n");
#endif

	return 0;
}

static int sunxi_rproc_prepare(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem, *tmp;
	struct reserved_mem *rmem;
	int index = 0;
	int ret;
	u64 da;

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
	struct elf32_hdr *ehdr  = (struct elf32_hdr *)fw->data;
	int ret;

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	chip->rproc_priv->pc_entry = ehdr->e_entry;

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
	if (err < 0)
		dev_err(&rproc->dev, "%s,%d kick err:%d\n",
			__func__, __LINE__, err);
	return;
}

static int sunxi_rproc_elf_find_section(struct rproc *rproc,
					 const struct firmware *fw,
					 const char *section_name,
					 struct elf32_shdr **find_shdr)
{
	struct device *dev = &rproc->dev;
	struct elf32_hdr *ehdr;
	struct elf32_shdr *shdr;
	const char *name_table;
	const u8 *elf_data = fw->data;
	u32 i, size;

	ehdr = (struct elf32_hdr *)elf_data;
	shdr = (struct elf32_shdr *)(elf_data + ehdr->e_shoff);

	name_table = elf_data + shdr[ehdr->e_shstrndx].sh_offset;
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		size = shdr->sh_size;

		if (strcmp(name_table + shdr->sh_name, section_name))
			continue;

		*find_shdr = shdr;
		dev_dbg(dev, "%s,%d %s addr 0x%x, size 0x%x\n",
			__func__, __LINE__, section_name, shdr->sh_addr, size);

		return 0;
	}

	return -EINVAL;

}

static int sunxi_rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc *chip = rproc->priv;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	struct elf32_shdr *shdr;
	int i, ret = 0;
	const u8 *elf_data = fw->data;
	u32 offset, da, memsz, filesz;
	void *ptr;

	/* get version from elf  */
	ret = sunxi_rproc_elf_find_section(rproc, fw, ".version_table", &shdr);
	if (ret) {
		dev_warn(dev, "%s,%d find segments err\n", __func__, __LINE__);
		/* Lack of ".version_table" should not be assumed as an error */
		ret = 0;
	} else {
		dev_info(dev, "firmware version: %s\n", elf_data + shdr->sh_offset);
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

		da = shdr->sh_addr;
		memsz = shdr->sh_size;
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			return -EINVAL;
		}
		memcpy(ptr, elf_data + shdr->sh_offset, memsz);

		return 0;
	}

	/* get ehdr & phdr addr */
	ehdr = (struct elf32_hdr *)elf_data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	/* arm can write/read local ram */
	sunxi_rproc_priv_set_localram(chip->rproc_priv, 1);

	dev_dbg(dev, "%s,%d\n", __func__, __LINE__);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		offset = phdr->p_offset;

		if (phdr->p_type != PT_LOAD)
			continue;

		dev_dbg(dev, "phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
			phdr->p_type, da, memsz, filesz);

		if ((memsz == 0) || (filesz == 0))
			continue;

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, da, memsz, NULL);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + phdr->p_offset, filesz);

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

static struct resource_table *sunxi_rproc_get_loaded_rsc_table(struct rproc *rproc, size_t *table_sz)
{
	struct sunxi_rproc *chip = rproc->priv;
	struct device *dev = rproc->dev.parent;
	const struct firmware *firmware_p;
	struct elf32_shdr *shdr;
	int ret;

	dev_dbg(dev, "get_loaded_rsc_table %s\n", rproc->firmware);

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	if (rproc->state == RPROC_DETACHED) {
		if (rproc->auto_boot) {
			ret = sunxi_request_firmware(&firmware_p, rproc->firmware, dev);
		} else {
			dev_info(dev, "rproc has been booted by another entity(perhaps bootloader), "
				"please confirm whether the ELF file is same!\n");
			ret = request_firmware(&firmware_p, rproc->firmware, dev);
		}
	} else {
		ret = request_firmware(&firmware_p, rproc->firmware, dev);
	}
#else
	if (rproc->state == RPROC_DETACHED) {
		dev_info(dev, "rproc has been booted by another entity(perhaps bootloader), "
			"please confirm whether the ELF file is same!\n");
	}
	ret = request_firmware(&firmware_p, rproc->firmware, dev);
#endif
	if (ret < 0) {
		dev_err(dev, "request_firmware failed: %d\n", ret);
		return ERR_PTR(-ENODEV);
	}

	ret = sunxi_rproc_elf_find_section(rproc, firmware_p,
							 ".resource_table", &shdr);
	if (ret) {
		dev_err(dev, "%s,%d find segments err\n", __func__, __LINE__);
		release_firmware(firmware_p);
		return ERR_PTR(-ENODEV);
	}

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	if (rproc->state == RPROC_DETACHED) {
		chip->rproc_priv->pc_entry = rproc_elf_get_boot_addr(rproc, firmware_p);
	}
#endif

	*table_sz = shdr->sh_size;
	chip->rsc_table_va = rproc_da_to_va(rproc, shdr->sh_addr, *table_sz, NULL);
	if (IS_ERR_OR_NULL(chip->rsc_table_va)) {
		dev_err(dev, "Unable to find loaded rsc table: %s\n", rproc->firmware);
		chip->rsc_table_va = NULL;
		release_firmware(firmware_p);
		return ERR_PTR(-ENOMEM);
	}

	dev_info(dev, "loaded resource table, addr: 0x%08x, size: %zu\n", shdr->sh_addr, *table_sz);

	release_firmware(firmware_p);

	return (struct resource_table *)chip->rsc_table_va;
}

static u64 suxni_rproc_elf_get_boot_addr(struct rproc *rproc, const struct firmware *fw)
{
	u64 data = 0;
	data = rproc_elf_get_boot_addr(rproc, fw);
	dev_dbg(&rproc->dev, "%s,%d elf boot addr = 0x%llx\n", __func__, __LINE__, data);
	return data;
}

extern struct dentry *sunxi_rproc_create_aw_trace_file(const char *name, struct rproc *rproc,
				       struct rproc_debug_trace *trace);

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
	trace->tfile = sunxi_rproc_create_aw_trace_file(name, rproc, trace);
	if (!trace->tfile) {
		kfree(trace);
		return -EINVAL;
	}

	list_add_tail(&trace->node, &rproc->traces);

	rproc->num_traces++;

	dev_info(dev, "add trace mem '%s', da: 0x%08x, len: %u\n", name, rsc->da, rsc->len);
	return 0;
}

static int sunxi_rproc_handle_rsc(struct rproc *rproc, u32 rsc_type,
			       void *ptr, int offset, int avail)
{
	dev_info(rproc->dev.parent, "handle vendor resource, type: %u\n", rsc_type);
	if (rsc_type == RSC_AW_TRACE) {
		return sunxi_rproc_handle_aw_trace(rproc, ptr, offset, avail);
	}

	return RSC_IGNORED;
}

static struct rproc_ops sunxi_rproc_ops = {
	.prepare	= sunxi_rproc_prepare,
	.unprepare	= sunxi_rproc_unprepare,
	.start		= sunxi_rproc_start,
	.stop		= sunxi_rproc_stop,
	.attach		= sunxi_rproc_attach,
	.detach		= sunxi_rproc_detach,
	.da_to_va	= sunxi_rproc_da_to_va,
	.kick		= sunxi_rproc_kick,
	.parse_fw	= sunxi_rproc_parse_fw,
	.find_loaded_rsc_table = rproc_elf_find_loaded_rsc_table,
	.get_loaded_rsc_table = sunxi_rproc_get_loaded_rsc_table,
	.load		= sunxi_rproc_elf_load_segments,
	.get_boot_addr	= suxni_rproc_elf_get_boot_addr,
	.sanity_check	= rproc_elf_sanity_check,
	.handle_rsc = sunxi_rproc_handle_rsc,
};

static const struct of_device_id sunxi_rproc_match[] = {
	{ .compatible = "allwinner,hifi4-rproc", .data = "hifi4" },
	{ .compatible = "allwinner,e906-rproc", .data = "e906" },
	{ .compatible = "allwinner,e907-rproc", .data = "e907" },
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

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
	ret = of_property_read_string(dev->of_node, "share-irq", &chip->rproc_priv->share_irq);
	if (ret < 0) {
		dev_err(dev, "fail to get share-irq\n");
		return -EINVAL;
	}
#endif

#if IS_ENABLED(CONFIG_PM_SLEEP)
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
#endif

	return 0;
}

static void sunxi_rproc_resource_put(struct rproc *rproc, struct platform_device *pdev)
{
	struct sunxi_rproc *chip = rproc->priv;
	sunxi_rproc_priv_resource_put(chip->rproc_priv, pdev);

#if IS_ENABLED(CONFIG_PM_SLEEP)
	sunxi_rproc_standby_exit(chip->rproc_standby, pdev);
#endif

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
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	struct device_node *fw_np = NULL;
#endif
	struct rproc *rproc;
	const char *fw_name = NULL;
	int ret;

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
	chip = rproc->priv;
	chip->rproc = rproc;
	chip->name = (char *)of_id->data;

	ret = sunxi_rproc_resource_get(rproc, pdev);
	if (ret) {
		dev_err(dev, "Failed to get resource\n");
		goto free_rproc;
	}

	chip->is_booted = sunxi_rproc_priv_is_booted(chip->rproc_priv);
	if (chip->is_booted)
		rproc->state = RPROC_DETACHED;

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

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
	if (rproc->auto_boot) {
		if (rproc->state == RPROC_DETACHED) {
			fw_np = of_parse_phandle(np, "fw-region", 0);
			if (fw_np) {
				struct resource r;
				resource_size_t len;

				ret = of_address_to_resource(fw_np, 0, &r);
				if (!ret) {
					len = resource_size(&r);

					dev_info(dev, "register memory firmware('%s') for '%s', addr: 0x%llx, size: %llu\n",
						rproc->firmware, chip->name, r.start, len);
					ret = sunxi_register_memory_fw(rproc->firmware, r.start, len);
					if (ret < 0)
						dev_err(dev, "register memory firmware('%s') failed. ret: %d\n", rproc->firmware, ret);
				} else {
					dev_err(dev, "parse dt node '%s' failed, ret: %d\n", fw_np->full_name, ret);
				}
			}
		} else {
			/* If the booloader doesn't boot remoteproc(eg: firmware verify failed), we need to set auto_boot to false,
			 * otherwise kernel will crash since some module has not been initialized. It is a abnormal situation when
			 * fast boot mode is enable(remoteproc modlue init in early stage and set auto_boot property in dts).
			 */
			rproc->auto_boot = false;
		}
	}
#endif

	ret = rproc_add(rproc);
	if (ret) {
		dev_err(dev, "Failed to register rproc\n");
		goto free_mbox;
	}

	list_add(&chip->list, &sunxi_rproc_list);

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

#if IS_ENABLED(CONFIG_PM_SLEEP)
struct sunxi_rproc_standby *sunxi_rproc_get_standby_handler(struct sunxi_rproc *chip)
{
	struct sunxi_rproc_standby *rproc_standby = NULL;

	if (chip && chip->rproc_standby)
		rproc_standby = chip->rproc_standby;

	return rproc_standby;
}

static int sunxi_rproc_pm_prepare(struct device *dev)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_standby *rproc_standby = ddata->rproc_standby;

	if (!rproc_standby || !rproc_standby->ctrl_en)
		return 0;

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

	if (sunxi_rproc_priv_is_booted(chip->rproc_priv))
		sunxi_rproc_stop(rproc);

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

subsys_initcall(sunxi_rproc_init);
module_exit(sunxi_rproc_exit);
#else
module_platform_driver(sunxi_rproc_driver);
#endif

MODULE_DESCRIPTION("Allwinnertech Remote Processor Control Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPROC_VERSION);

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by vmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "scp_helper.h"
#include "scp.h"

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>
#include "scp_reservedmem_define.h"
#endif

phys_addr_t scp_mem_base_phys;
phys_addr_t scp_mem_base_virt;
phys_addr_t scp_mem_size;

/* set flag after driver initial done */
static bool driver_init_done;

/*
 * memory copy to scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_to_scp(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * memory copy from scp sram
 * @param trg: trg address
 * @param src: src address
 * @param size: memory size
 */
void memcpy_from_scp(void *trg, const void __iomem *src, int size)
{
	int i;
	u32 *t = trg;
	const u32 __iomem *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * register /dev and /sys files
 * @return:     0: success, otherwise: fail
 */
static int create_files(void)
{
	return 0;
}

#define SCP_MEM_RESERVED_KEY "cix,reserve-memory-scp_share"
int scp_reserve_mem_of_init(struct reserved_mem *rmem)
{
	pr_notice("[SCP]%s %pa %pa\n", __func__, &rmem->base, &rmem->size);
	scp_mem_base_phys = (phys_addr_t) rmem->base;
	scp_mem_size = (phys_addr_t) rmem->size;

	return 0;
}

RESERVEDMEM_OF_DECLARE(scp_reserve_mem_init
			, SCP_MEM_RESERVED_KEY, scp_reserve_mem_of_init);

phys_addr_t scp_get_reserve_mem_phys(enum scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_notice("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_phys;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_phys);

phys_addr_t scp_get_reserve_mem_virt(enum scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_notice("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].start_virt;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_virt);

phys_addr_t scp_get_reserve_mem_size(enum scp_reserve_mem_id_t id)
{
	if (id >= NUMS_MEM_ID) {
		pr_notice("[SCP] no reserve memory for %d", id);
		return 0;
	} else
		return scp_reserve_mblock[id].size;
}
EXPORT_SYMBOL_GPL(scp_get_reserve_mem_size);

static int scp_reserve_memory_ioremap(struct platform_device *pdev)
{
	struct device dev = pdev->dev;
	struct device_node *np = dev.of_node;
	struct of_phandle_iterator it;
	struct reserved_mem *rmem;

	/* Register associated reserved memory regions */
	of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	while (of_phandle_iterator_next(&it) == 0) {
		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			pr_err("unable to acquire memory-region\n");
			return -EINVAL;
		}

		if (!strcmp(it.node->name, "sfh_sharebuffer")) {
			scp_reserve_mblock[SENS_MEM_ID].start_phys = (phys_addr_t) rmem->base;
			scp_reserve_mblock[SENS_MEM_ID].size = (phys_addr_t) rmem->size;
			scp_reserve_mblock[SENS_MEM_ID].start_virt = (phys_addr_t)(size_t)ioremap_wc(scp_reserve_mblock[SENS_MEM_ID].start_phys,
					scp_reserve_mblock[SENS_MEM_ID].size);
		} else {
			/* to do */
		}

		pr_notice("add carveout %s, base 0x%llx, size 0x%llx\n", it.node->name, rmem->base, rmem->size);
	}

	return 0;
}

void scp_region_info_init(void) {}

void scp_recovery_init(void)
{
 //To do
}

static int scp_device_probe(struct platform_device *pdev)
{
	int ret = 0;

	/*scp resvered memory*/
	pr_notice("[SCP] scp_reserve_memory_ioremap\n");
	ret = scp_reserve_memory_ioremap(pdev);
	if (ret) {
		pr_notice("[SCP]scp_reserve_memory_ioremap failed\n");
		return ret;
	}

	return ret;
}

static int scp_device_remove(struct platform_device *dev)
{
	return 0;
}

static const struct of_device_id scp_of_ids[] = {
	{ .compatible = "cix,sfh_scp", },
	{}
};

static struct platform_driver cix_scp_device = {
	.probe = scp_device_probe,
	.remove = scp_device_remove,
	.driver = {
		.name = "scp",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = scp_of_ids,
#endif
	},
};

/*
 * driver initialization entry point
 */
static int __init scp_init(void)
{
	int ret = 0;

	/* scp platform initialise */
	pr_debug("[SCP] %s begins\n", __func__);

	if (platform_driver_register(&cix_scp_device))
		pr_notice("[SCP] scp probe fail\n");

	/* scp platform initialise */
	scp_region_info_init();
	pr_debug("[SCP] platform init\n");

	/* scp sysfs initialise */
	pr_debug("[SCP] sysfs init\n");
	ret = create_files();
	if (unlikely(ret != 0)) {
		pr_notice("[SCP] create files failed\n");
		goto err;
	}

	scp_recovery_init();

	driver_init_done = true;

err:
	return ret;
}

/*
 * driver exit point
 */
static void __exit scp_exit(void)
{

}

device_initcall_sync(scp_init);
module_exit(scp_exit);

MODULE_DESCRIPTION("MEDIATEK Module SCP driver");
MODULE_AUTHOR("Mediatek");
MODULE_LICENSE("GPL v2");

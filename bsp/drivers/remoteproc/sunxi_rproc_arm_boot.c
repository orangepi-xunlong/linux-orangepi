/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#if IS_ENABLED(CONFIG_ARM64)
#include <asm/sysreg.h>
#endif
#include <asm/cputype.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>
#include <asm/io.h>
#include <linux/remoteproc.h>
#include <linux/mailbox_client.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/of_reserved_mem.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/arm-smccc.h>
#include <linux/cpu.h>
#include <linux/device.h>
#include <sunxi-smc.h>
#include "sunxi_rproc_boot.h"
#include "sunxi_rproc_standby.h"


static int sunxi_rproc_arm_reset(struct sunxi_rproc_priv *rproc_priv);

#define CPU_OFF_IRQ    (10)
#define DEAD_TIMEOUT			msecs_to_jiffies(1000)

#if IS_ENABLED(CONFIG_ARM64)
#define PSCI_FN_BASE                    (0xc4000000U)
#define PSCI_FN(n)                      (PSCI_FN_BASE + (n))
#define PSCI_CPU_OFF                    (0x84000002U)
#define PSCI_CPU_ON                     PSCI_FN(3)
#define PSCI_CPU_STATE                  (0x8000ff42U)

static int sunxi_rproc_arm_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value);
static int sunxi_rproc_arm_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value);

#define ICC_SGI1R_SGI_ID_SHIFT           24
#define ICC_SGI1R_RS_SHIFT               44

#define ICC_SGI1R_TARGET_LIST_SHIFT     0
#define ICC_SGI1R_AFFINITY_1_SHIFT      16
#define ICC_SGI1R_SGI_ID_SHIFT          24
#define ICC_SGI1R_AFFINITY_2_SHIFT      32
#define ICC_SGI1R_IRQ_ROUTING_MODE_BIT  40
#define ICC_SGI1R_RS_SHIFT              44
#define ICC_SGI1R_AFFINITY_3_SHIFT      48

#define MPIDR_RS(mpidr)                        (((mpidr) & 0xF0UL) >> 4)

#define MPIDR_TO_SGI_RS(mpidr)        (MPIDR_RS(mpidr) << ICC_SGI1R_RS_SHIFT)

#define MPIDR_TO_SGI_AFFINITY(cluster_id, level) (MPIDR_AFFINITY_LEVEL(cluster_id, level) << ICC_SGI1R_AFFINITY_## level ##_SHIFT)

static void send_sgi_to_amp_os(struct sunxi_rproc_arm_cfg *cfg,
				u16 tlist, unsigned int irq)
{
	u64 val;
	u64 cluster_id = cfg->cpu_id;

	val = (MPIDR_TO_SGI_AFFINITY(cluster_id, 3)	|
	       MPIDR_TO_SGI_AFFINITY(cluster_id, 2)	|
	       irq << ICC_SGI1R_SGI_ID_SHIFT		|
	       MPIDR_TO_SGI_AFFINITY(cluster_id, 1)	|
	       MPIDR_TO_SGI_RS(cluster_id)		|
	       tlist << ICC_SGI1R_TARGET_LIST_SHIFT);

	write_sysreg_s(val, SYS_ICC_SGI1R_EL1);
}

static int check_os_online(struct sunxi_rproc_arm_cfg *cfg)
{
	struct arm_smccc_res res;
	printk("cpu-id %x \n", cfg->cpu_id);
	arm_smccc_smc(PSCI_CPU_STATE, cfg->cpu_id, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

#else
#define PSCI_CPU_ON             (0x84000003U)
#define PSCI_CPU_STATE                  (0x8000ff42U)
#define GICD_SGIR       0xF00

enum optee_amp_subcmd {
	AMP_CPU_ON_UBOOT = 1,
	AMP_CPU_ON_KERNEL,
	AMP_CPU_OFF,
	AMP_CPU_STATE,
};

enum core_state_id {
	CORE_OFF = 0,
	CORE_RET,
	CORE_AWAKE,
	CORE_ON,
};

static void send_sgi_to_amp_os(struct sunxi_rproc_arm_cfg *cfg,
				u16 tlist, unsigned int irq)
{
	int target_cpu = cfg->cpu_id;
	uint32_t target_list = (1 << target_cpu);
	uint32_t val = (target_list << 16) | (irq & 0x0F);

	writel(val, cfg->gic_dist + GICD_SGIR);
}

static int check_os_online(struct sunxi_rproc_arm_cfg *cfg)
{
	uint32_t val;
	val = sunxi_optee_call_amp(AMP_CPU_STATE, cfg->cpu_id, 0);

	if (CORE_OFF == val)
		return 0;
	return 1;
}
#endif

static void dead_work_func(struct work_struct *work)
{
	int ret;
	struct delayed_work *dead_work = to_delayed_work(work);
	struct sunxi_rproc_arm_cfg *cfg;

	cfg = container_of(dead_work, struct sunxi_rproc_arm_cfg, dead_work);

	ret = check_os_online(cfg);

	/* ret == 0, mean cpu is off  */
	if (!ret) {
		complete(&cfg->complete_dead);
	} else
		schedule_delayed_work(&cfg->dead_work, msecs_to_jiffies(50));
}

static int devm_sunxi_rproc_arm_resource_get(struct sunxi_rproc_priv *rproc_priv,
					struct platform_device *pdev)
{
	struct sunxi_rproc_arm_cfg *arm_cfg;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *amp_node = NULL;
	__maybe_unused struct device_node *gic_node;
	__maybe_unused struct resource res;
	u32 *map_array;
	int ret, i;

	rproc_priv->dev = dev;

	arm_cfg = devm_kzalloc(dev, sizeof(*arm_cfg), GFP_KERNEL);
	if (!arm_cfg) {
		dev_err(dev, "alloc arm cfg error\n");
		return -ENOMEM;
	}

	amp_node = of_parse_phandle(np, "boot-amp", 0);
	if (!amp_node) {
		dev_err(dev, "fail to get boot-amp\n");
		return -ENXIO;
	}
	ret = of_property_read_u32(amp_node, "cpu-id", &arm_cfg->cpu_id);
	if (ret < 0) {
		dev_err(dev, "no \"cpu-id\" node specified\n");
		return -ENXIO;
	}

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		return -ENXIO;
	}
	rproc_priv->mem_maps_cnt = ret;
	rproc_priv->mem_maps = devm_kcalloc(dev, rproc_priv->mem_maps_cnt,
				       sizeof(struct sunxi_rproc_memory_mapping),
				       GFP_KERNEL);
	if (!rproc_priv->mem_maps)
		return -ENOMEM;

	map_array = devm_kcalloc(dev, rproc_priv->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
					 rproc_priv->mem_maps_cnt * 3);
	if (ret) {
		dev_err(dev, "fail to read memory-mappings\n");
		return -ENXIO;
	}

	for (i = 0; i < rproc_priv->mem_maps_cnt; i++) {
		rproc_priv->mem_maps[i].da = map_array[i * 3];
		rproc_priv->mem_maps[i].len = map_array[i * 3 + 1];
		rproc_priv->mem_maps[i].pa = map_array[i * 3 + 2];
		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%llx, pa: 0x%llx\n",
			i, rproc_priv->mem_maps[i].da, rproc_priv->mem_maps[i].len,
			rproc_priv->mem_maps[i].pa);
	}

	devm_kfree(dev, map_array);

#if !IS_ENABLED(CONFIG_ARM64)
	gic_node = of_find_compatible_node(NULL, NULL, "arm,cortex-a7-gic");
	if (!gic_node) {
		printk(KERN_ERR "Failed to find GIC node\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(gic_node, 0, &res);
	if (ret) {
		printk(KERN_ERR "Failed to get GIC distributor address\n");
		of_node_put(gic_node);
		return ret;
	}

	arm_cfg->gic_dist = (void __iomem *)ioremap(res.start, resource_size(&res));
	if (!arm_cfg->gic_dist) {
		printk(KERN_ERR "Failed to ioremap GIC distributor base address\n");
		of_node_put(gic_node);
		return -ENOMEM;
	}

	of_node_put(gic_node);
#endif
	INIT_DELAYED_WORK(&arm_cfg->dead_work, dead_work_func);
	init_completion(&arm_cfg->complete_dead);

	rproc_priv->rproc_cfg = arm_cfg;

	return 0;
}


static void devm_sunxi_rproc_arm_resource_put(struct sunxi_rproc_priv *rproc_priv,
					struct platform_device *pdev)
{
	struct sunxi_rproc_arm_cfg *arm_cfg = rproc_priv->rproc_cfg;

	cancel_delayed_work_sync(&arm_cfg->dead_work);
	reinit_completion(&arm_cfg->complete_dead);

	return;
}

static int sunxi_rproc_arm_attach(struct sunxi_rproc_priv *rproc_priv)
{

	return 0;
}

static int sunxi_rproc_arm_cpu_hotplug(int cpu_id, bool online)
{
	int ret, reg_len;
	int i = 0, logic_cpu = 0;
	struct device_node *cpus_node;
	struct device_node *cpu_node;
	struct device *cpu_dev;
	unsigned long addr;
	const __be32 *prop;

	cpus_node = of_find_node_by_path("/cpus");
	if (!cpus_node) {
		printk(KERN_ERR "/cpus node not found\n");
		return -ENODEV;
	}

	for_each_child_of_node(cpus_node, cpu_node) {
		prop = of_get_property(cpu_node, "reg", &reg_len);
		if (prop) {
			if (reg_len == 8)
				of_property_read_u64(cpu_node, "reg", (u64 *)&addr);
			else
				of_property_read_u32(cpu_node, "reg", (u32 *)&addr);
			if (addr == cpu_id) {
				logic_cpu = i;
				break;
			}
			i++;
		}
	}

	of_node_put(cpus_node);

	cpu_dev = get_cpu_device(logic_cpu);
	if (!cpu_dev) {
		pr_err("can`t found cpu(%x) cpu_dev \n", cpu_id);
		return -1;
	}

	ret = online ? device_online(cpu_dev) : device_offline(cpu_dev);

	return ret;
}

static int sunxi_rproc_arm_start(struct sunxi_rproc_priv *rproc_priv)
{
	struct sunxi_rproc_arm_cfg *cfg = rproc_priv->rproc_cfg;
	struct device *dev = rproc_priv->dev;
	struct arm_smccc_res res;
	int ret;
	//u32 reg_val;

	dev_err(dev, "%s,%d\n", __func__, __LINE__);

	sunxi_rproc_arm_cpu_hotplug(cfg->cpu_id, false);	/* cpu offline in kernel */

	/* reset arm */
	ret = sunxi_rproc_arm_reset(rproc_priv);
	if (ret) {
		dev_err(dev, "rproc reset err\n");
		return ret;
	}

#if IS_ENABLED(CONFIG_ARM64)
	arm_smccc_smc(PSCI_CPU_ON, cfg->cpu_id, rproc_priv->pc_entry, 0, 0, 0, 0, 0, &res);
#else
	arm_smccc_smc(PSCI_CPU_ON, cfg->cpu_id, rproc_priv->pc_entry, 0, 0, 0, 0, 0, &res);
#endif

	dev_err(dev, "%s,%d, PSCI_CPU_ON %x, start addr 0x%x\n", __func__, __LINE__,
			PSCI_CPU_ON, rproc_priv->pc_entry);
	return 0;
}

static int sunxi_rproc_arm_stop(struct sunxi_rproc_priv *rproc_priv)
{
	int ret;
	struct sunxi_rproc_arm_cfg *cfg = rproc_priv->rproc_cfg;

	dev_dbg(rproc_priv->dev, "%s,%d\n", __func__, __LINE__);

	ret = sunxi_rproc_arm_reset(rproc_priv);
	if (ret) {
		dev_err(rproc_priv->dev, "rproc reset err\n");
		return ret;
	}

	ret = check_os_online(cfg);
	if (!ret) {
		dev_err(rproc_priv->dev, "amp cpu %x have offline \n", cfg->cpu_id);
		return 0;
	}

	dev_info(rproc_priv->dev, "amp cpu %x run ---- \n", cfg->cpu_id);

	send_sgi_to_amp_os(cfg, 1, CPU_OFF_IRQ);

	schedule_delayed_work(&cfg->dead_work, msecs_to_jiffies(50));
	ret = wait_for_completion_timeout(&cfg->complete_dead, DEAD_TIMEOUT);
	cancel_delayed_work_sync(&cfg->dead_work);
	if (!ret) {
		dev_err(rproc_priv->dev, "timeout return cpu dead\n");
		return -ETIME;
	}

	sunxi_rproc_arm_cpu_hotplug(cfg->cpu_id, true);	/* cpu online in kernel */


	return 0;
}

static int sunxi_rproc_arm_reset(struct sunxi_rproc_priv *rproc_priv)
{
	return 0;
}

static int sunxi_rproc_arm_set_localram(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static int sunxi_rproc_arm_set_runstall(struct sunxi_rproc_priv *rproc_priv, u32 value)
{
	return 0;
}

static bool sunxi_rproc_arm_is_booted(struct sunxi_rproc_priv *rproc_priv)
{
	struct device_node *np = rproc_priv->np;
	rproc_priv->auto_boot = of_property_read_bool(np, "auto-boot");

	return rproc_priv->auto_boot;
}

static struct sunxi_rproc_ops sunxi_rproc_arm_ops = {
	.resource_get = devm_sunxi_rproc_arm_resource_get,
	.resource_put = devm_sunxi_rproc_arm_resource_put,
	.attach = sunxi_rproc_arm_attach,
	.start = sunxi_rproc_arm_start,
	.stop = sunxi_rproc_arm_stop,
	.reset = sunxi_rproc_arm_reset,
	.set_localram = sunxi_rproc_arm_set_localram,
	.set_runstall = sunxi_rproc_arm_set_runstall,
	.is_booted = sunxi_rproc_arm_is_booted,
};

/* arm_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_arm_rtos_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("arm_rtos", &sunxi_rproc_arm_ops, NULL);
	if (ret) {
		pr_err("arm register ops failed\n");
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_arm_rtos_boot_init);

static void __exit sunxi_rproc_arm_rtos_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister("arm_rtos");
	if (ret)
		pr_err("arm unregister ops failed\n");
}
module_exit(sunxi_rproc_arm_rtos_boot_exit)

/* arm_boot_init must run before sunxi_rproc probe */
static int __init sunxi_rproc_arm_baremetal_boot_init(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_register("arm_baremetal",
					&sunxi_rproc_arm_ops, NULL);
	if (ret) {
		pr_err("arm register ops failed\n");
		return ret;
	}

	return 0;
}
subsys_initcall(sunxi_rproc_arm_baremetal_boot_init);

static void __exit sunxi_rproc_arm_baremetal_boot_exit(void)
{
	int ret;

	ret = sunxi_rproc_priv_ops_unregister("arm_baremetal");
	if (ret)
		pr_err("arm unregister ops failed\n");
}
module_exit(sunxi_rproc_arm_baremetal_boot_exit)


MODULE_DESCRIPTION("Allwinner sunxi rproc arm boot driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");

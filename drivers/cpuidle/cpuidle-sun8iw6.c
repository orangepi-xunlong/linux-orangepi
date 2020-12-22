/* linux/arch/arm/mach-sunxi/cpuidle.c
 *
 * Copyright (C) 2013-2014 allwinner.
 * kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/time.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/suspend.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/clockchips.h>
#include <linux/sched.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/arisc/arisc.h>
#include <linux/arisc/hwspinlock.h>
#include <linux/earlysuspend.h>
#include <linux/cpu.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <asm/smp_plat.h>
#include <asm/delay.h>
#include <asm/cp15.h>
#include <asm/suspend.h>
#include <asm/cacheflush.h>
#include <asm/hardware/gic.h>
#include <asm/cpuidle.h>
#include <mach/cpuidle-sunxi.h>
#include <mach/sun8i/platsmp.h>

#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
static struct clk *usb_hci_clk0;
static struct clk *usb_hci_clk1;
static struct clk *usb_otg_clk;
/* the clk of usb phy is always 24M */
static struct clk *sdmmc_clk0;
static struct clk *sdmmc_clk1;
static struct clk *sdmmc_clk2;
static struct clk *nand_clk0;
static struct clk *dma_clk;
/* usb */
#define ECHI_PROT_STAT	(0x54)
#define OCHI_PORT_STAT	(0x454)
#define	OTG_PORT_STAT	(0x4c)
/* sdmmc */
#define SDC_DMAC_STAT	(0x88)
/* dma */
#define DMAC_CHAN_STAT	(0x30)
/* ndfc */
#define NAND_STAT	(0x4)

#define ENCNT(__clkp) (__clkp->enable_count)

static int dram_master_clk_get(void)
{
	usb_hci_clk0 = clk_get(NULL, USBEHCI0_CLK);
	if (IS_ERR(usb_hci_clk0)) {
		pr_err("%s get %s clk failed!\n", __func__, USBEHCI0_CLK);
		return -1;
	}
	usb_hci_clk1 = clk_get(NULL, USBEHCI1_CLK);
	if (IS_ERR(usb_hci_clk1)) {
		pr_err("%s get %s clk failed!\n", __func__, USBEHCI1_CLK);
		return -1;
	}
	usb_otg_clk = clk_get(NULL, USBOTG_CLK);
	if (IS_ERR(usb_otg_clk)) {
		pr_err("%s get %s clk failed!\n", __func__, USBOTG_CLK);
		return -1 ;
	}

	sdmmc_clk0 = clk_get(NULL, SDMMC0_CLK);
	if (IS_ERR(sdmmc_clk0)) {
		pr_err("%s get %s clk failed!\n", __func__, SDMMC0_CLK);
		return -1;
	}
	sdmmc_clk1 = clk_get(NULL, SDMMC1_CLK);
	if (IS_ERR(sdmmc_clk1)) {
		pr_err("%s get %s clk failed!\n", __func__, SDMMC1_CLK);
		return -1;
	}
	sdmmc_clk2 = clk_get(NULL, SDMMC2_CLK);
	if (IS_ERR(sdmmc_clk2)) {
		pr_err("%s get %s clk failed!\n", __func__, SDMMC2_CLK);
		return -1;
	}

	nand_clk0 = clk_get(NULL, NAND_CLK);
	if (IS_ERR(nand_clk0)) {
		pr_err("%s get %s clk failed!\n", __func__, NAND_CLK);
		return -1;
	}

	dma_clk = clk_get(NULL, DMA_CLK);
	if (IS_ERR(dma_clk)) {
		pr_err("%s get %s clk failed!\n", __func__, DMA_CLK);
		return -1;
	}

	return 0;
}

static int dram_master_check(void)
{
	int reg_val = 0;

	if (ENCNT(nand_clk0)) {
		reg_val |= readl(SUNXI_NFC0_VBASE + NAND_STAT) & (0x1<<4);
		if (reg_val)
			return -1;
	}
	if (ENCNT(usb_hci_clk0)) {
		reg_val |= readl(SUNXI_USB_HCI0_VBASE + ECHI_PROT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI1_VBASE + ECHI_PROT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI0_VBASE + OCHI_PORT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI1_VBASE + OCHI_PORT_STAT) & (0x1<<0);
		if (reg_val)
			return -2;
	}
	if (ENCNT(usb_otg_clk)) {
		reg_val |= readl(SUNXI_USB_OTG_VBASE + OTG_PORT_STAT) & (0x0ff<<0);
		if (reg_val)
			return -3;
	}
	if (ENCNT(sdmmc_clk0))
		reg_val |= readl(SUNXI_SDMMC0_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if (ENCNT(sdmmc_clk1))
		reg_val |= readl(SUNXI_SDMMC1_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if (ENCNT(sdmmc_clk2))
		reg_val |= readl(SUNXI_SDMMC2_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if (reg_val)
		return -4;

	/* there is no needed to check dma state for audio(dma user)
	 * has used sram instead of dram
	 */
	//if (ENCNT(dma_clk))
		//reg_val |= readl(SUNXI_DMA_VBASE + DMAC_CHAN_STAT) & (0x0ff<<0);

	return reg_val;
}
#endif

#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
static int cpuidle_early_suspend_flag = 0;
static struct early_suspend cpuidle_early_suspend;

static void sunxi_cpuidle_earlysuspend(struct early_suspend *h)
{
	pr_debug("%s:enter\n", __func__);
	cpuidle_early_suspend_flag = 1;
}

static void sunxi_cpuidle_lateresume(struct early_suspend *h)
{
	pr_debug("%s:resume\n", __func__);
	cpuidle_early_suspend_flag = 0;
}
#endif

/* C2 State Flags */
#define CPUIDLE_FLAG_C2_STATE   (1<<16) /* into c2 state */
/* dram enter self-refreash flg */
#define DRAM_SELFREFLASH        (1<<0)

static DEFINE_PER_CPU(struct cpuidle_device, sunxi_cpuidle_device);

static int sunxi_enter_c0state(struct cpuidle_device *dev, \
                               struct cpuidle_driver *drv, int index)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	cpu_do_idle();

	do_gettimeofday(&after);
	local_irq_enable();

	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
	            (after.tv_usec - before.tv_usec);
	dev->last_residency = idle_time < 0 ? 0 : idle_time;

	return index;
}

/*
 * notrace prevents trace shims from getting inserted where they
 * should not. Global jumps and ldrex/strex must not be inserted
 * in power down sequences where caches and MMU may be turned off.
 */
static int notrace sunxi_powerdown_finisher(unsigned long flg)
{
	/* MCPM works with HW CPU identifiers */
	unsigned int mpidr = read_cpuid_mpidr();
	unsigned int cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	unsigned int cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);

	mcpm_set_entry_vector(cpu, cluster, cpu_resume);

	/*
	 * Residency value passed to mcpm_cpu_suspend back-end
	 * has to be given clear semantics. Set to 0 as a
	 * temporary value.
	 */
	/* write 0 after gic_cpu_out() */
	writel(0xff, CLUSTER_CPUW_FLG(cluster, cpu));
	mcpm_cpu_suspend(mpidr | flg);

	/* return value != 0 means failure */
	return 1;
}

static int sunxi_cpu_core_power_down(struct cpuidle_device *dev, \
                                     struct cpuidle_driver *drv, int index)
{
	cpu_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();

	cpu_suspend(0, sunxi_powerdown_finisher);

	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	cpu_pm_exit();

	return index;
}

/*
 * sunxi_enter_c1state - Programs CPU to enter the specified state
 * @dev: cpuidle device
 * @drv: The target state to be programmed
 * @idx: state index
 *
 * Called from the CPUidle framework to program the device to the
 * specified target state selected by the governor.
 */
static int sunxi_enter_c1state(struct cpuidle_device *dev, \
                               struct cpuidle_driver *drv, int index)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	sunxi_cpu_core_power_down(dev, drv, index);

	do_gettimeofday(&after);
	local_irq_enable();

	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
	            (after.tv_usec - before.tv_usec);
	dev->last_residency = idle_time;

	return index;
}

/*
 * We can't use regular spinlocks. In the switcher case, it is possible
 * for an outbound CPU to call power_down() after its inbound counterpart
 * is already live using the same logical CPU number which trips lockdep
 * debugging.
 */
static arch_spinlock_t sun8iw6_mcpm_lock = __ARCH_SPIN_LOCK_UNLOCKED;

/* sunxi cluster and cpu use status, this is use to detect the last-man
 * for the last-man should disable cci. cpu use count used for debug.
 */
static int sun8iw6_cpu_use_count[MAX_NR_CLUSTERS][MAX_CPUS_PER_CLUSTER];
static volatile int sun8iw6_cluster_use_count[MAX_NR_CLUSTERS];

/*
 * notrace prevents trace shims from getting inserted where they
 * should not. Global jumps and ldrex/strex must not be inserted
 * in power down sequences where caches and MMU may be turned off.
 */
static int notrace sunxi_powerdown_c2_finisher(unsigned long flg)
{
	/* MCPM works with HW CPU identifiers */
	unsigned int mpidr = read_cpuid_mpidr();
	unsigned int cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	unsigned int cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);
	bool last_man = false;
	struct sunxi_cpuidle_para sunxi_idle_para;

	mcpm_set_entry_vector(cpu, cluster, cpu_resume);

	arch_spin_lock(&sun8iw6_mcpm_lock);
	sun8iw6_cluster_use_count[cluster]--;
	sun8iw6_cpu_use_count[cluster][cpu]--;
	/* check is the last-man, and set flg */
	if (sun8iw6_cluster_use_count[cluster] == 0) {
		writel(0xff, CLUSTER_CPUX_FLG(cluster, cpu));
		last_man = true;
	}
	arch_spin_unlock(&sun8iw6_mcpm_lock);

	/* call cpus to power off */
	sunxi_idle_para.mpidr = (unsigned long)mpidr;
	writel(0xff, CLUSTER_CPUW_FLG(cluster, cpu));
#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
	if (last_man && cpuidle_early_suspend_flag && !dram_master_check())
		sunxi_idle_para.flags = DRAM_SELFREFLASH | flg;
	else
#endif
		sunxi_idle_para.flags = flg;

	arisc_enter_cpuidle(NULL, NULL, &sunxi_idle_para);

	if (last_man) {
		int t = 0;

		/* wait for cpus received this message and respond,
		 * for reconfirm is this cpu the last man truelly, then clear flg
		 */
		while (1) {
			udelay(2);
			if (readl(CLUSTER_CPUS_FLG(cluster, cpu)) == 2) {
				writel(0, CLUSTER_CPUX_FLG(cluster, cpu));
				break; /* last_man is true */
			} else if (readl(CLUSTER_CPUS_FLG(cluster, cpu)) == 4) {
				writel(0, CLUSTER_CPUX_FLG(cluster, cpu));
				goto out; /* last_man is false */
			}
			if(++t > 10000) {
				pr_warn("cpu%didle time out!\n",  \
				        cluster * 4 + cpu);
				t = 0;
			}
		}
		sunxi_idle_cluster_die(cluster);
	}
out:
	sunxi_idle_cpu_die();

	/* return value != 0 means failure */
	return 1;
}

static int sunxi_cpu_power_down_c2state(struct cpuidle_device *dev, \
                                        struct cpuidle_driver *drv, \
                                        int index)
{
	unsigned int mpidr = read_cpuid_mpidr();
	unsigned int cluster = MPIDR_AFFINITY_LEVEL(mpidr, 1);
	unsigned int cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);

	cpu_pm_enter();
	//cpu_cluster_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();

	cpu_suspend(CPUIDLE_FLAG_C2_STATE, sunxi_powerdown_c2_finisher);

	/*
	 * Since this is called with IRQs enabled, and no arch_spin_lock_irq
	 * variant exists, we need to disable IRQs manually here.
	 */
	local_irq_disable();

	arch_spin_lock(&sun8iw6_mcpm_lock);
	sun8iw6_cluster_use_count[cluster]++;
	sun8iw6_cpu_use_count[cluster][cpu]++;
	arch_spin_unlock(&sun8iw6_mcpm_lock);

	local_irq_enable();

	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	//cpu_cluster_pm_exit();
	cpu_pm_exit();

	return index;
}

static int sunxi_enter_c2state(struct cpuidle_device *dev, \
                               struct cpuidle_driver *drv, int index)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	sunxi_cpu_power_down_c2state(dev, drv, index);

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC + \
	            (after.tv_usec - before.tv_usec);
	dev->last_residency = idle_time < 0 ? 0 : idle_time;

	return index;
}

/*
 * @enter: low power process function
 * @exit_latency: latency of exit this state, based on us
 * @power_usage: power used by cpu under this state, based on mw
 * @target_residency: the minimum of time should cpu spend in
 *   this state, based on us
 */
static struct cpuidle_driver sunxi_idle_driver = {
	.name                           = "sunxi_idle",
	.owner                          = THIS_MODULE,
	.states[0] = {
		.enter                  = sunxi_enter_c0state,
		.exit_latency           = 1,
		.target_residency       = 100,
		.power_usage            = 1000,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "C0",
		.desc                   = "ARM (WFI)",
	},
	.states[1] = {
		.enter                  = sunxi_enter_c1state,
		.exit_latency           = 3000,
		.target_residency       = 10000,
		.power_usage            = 500,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "C1",
		.desc                   = "SUNXI CORE POWER DOWN",
	},
	.states[2] = {
		.enter                  = sunxi_enter_c2state,
		.exit_latency           = 10000,
		.target_residency       = 20000,
		.power_usage            = 100,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "C2",
		.desc                   = "SUNXI CLUSTER POWER DOWN",
	},
	.state_count = 3,
};

static int sun8i_cpuidle_state_init(void)
{
	unsigned int i, j;

	for (i = 0; i < MAX_NR_CLUSTERS; i++) {
		sun8iw6_cluster_use_count[i] = MAX_CPUS_PER_CLUSTER;
		for (j = 0; j < MAX_CPUS_PER_CLUSTER; j++)
			sun8iw6_cpu_use_count[i][j] = 1;
	}

	return 0;
}

#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG
#define CPU_2_MPIDR(cpu) ((cpu/4)<<MPIDR_LEVEL_BITS) | (cpu%4)

static int cpuidle_notify(struct notifier_block *self, unsigned long action, \
                          void *hcpu)
{
	unsigned int cluster, cpu;
	int need_sync = 0;
	struct sunxi_cpuidle_para sunxi_idle_para;

	cluster = (unsigned int)hcpu / MAX_CPUS_PER_CLUSTER;
	cpu = (unsigned int)hcpu % MAX_CPUS_PER_CLUSTER;
	arch_spin_lock(&sun8iw6_mcpm_lock);
	switch (action) {
	case CPU_UP_PREPARE:
		sun8iw6_cluster_use_count[cluster]++;
		sun8iw6_cpu_use_count[cluster][cpu]++;
		need_sync = 1;
		break;
	case CPU_POST_DEAD:
		sun8iw6_cluster_use_count[cluster]--;
		sun8iw6_cpu_use_count[cluster][cpu]--;
		need_sync = 2;
		break;
	case CPU_UP_CANCELED:
		sun8iw6_cluster_use_count[cluster]--;
		sun8iw6_cpu_use_count[cluster][cpu]--;
		need_sync = 2;
		break;
	}

	if (need_sync) {
		if (need_sync == 1)
			sunxi_idle_para.flags = 0x3<<24;
		else
			sunxi_idle_para.flags = 0x1<<24;
		sunxi_idle_para.mpidr = CPU_2_MPIDR((unsigned int)hcpu);
		arisc_config_cpuidle(NULL, NULL, &sunxi_idle_para);
		pr_debug("ns:%d,mpidr:%lx,cl_use_count[%d]:%d\n", need_sync, \
		         sunxi_idle_para.mpidr, cluster, \
		         sun8iw6_cluster_use_count[cluster]);
	}
	arch_spin_unlock(&sun8iw6_mcpm_lock);

	return NOTIFY_OK;
}

static struct notifier_block cpuidle_nb = {
	.notifier_call = cpuidle_notify,
};
#endif

static int __init sunxi_cpuidle_init(void)
{
	int err = 0;
	int cpu;
	struct cpuidle_device *device;

	sunxi_set_bootcpu_hotplugflg();
	sunxi_set_secondary_entry((void *)(virt_to_phys(mcpm_entry_point)));

	hwspinlock_ready_init();
	sun8i_cpuidle_state_init();

#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG
	err = register_cpu_notifier(&cpuidle_nb);
	if (err)
		goto err_out;
#endif

#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
	if (dram_master_clk_get()) {
		pr_err("cpuidle get modules' clk failed\n,");
		goto notify_err;
	}
#endif

#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
	cpuidle_early_suspend.suspend = sunxi_cpuidle_earlysuspend;
	cpuidle_early_suspend.resume = sunxi_cpuidle_lateresume;
	cpuidle_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 300;
	register_early_suspend(&cpuidle_early_suspend);
#endif

	sunxi_idle_driver.safe_state_index = 0;
	err = cpuidle_register_driver(&sunxi_idle_driver);
	if (err)
	 goto dram_err;

	for_each_possible_cpu(cpu) {
		device = &per_cpu(sunxi_cpuidle_device, cpu);
		device->cpu = cpu;
		err = cpuidle_register_device(device);
		if (err)
			goto drv_err;
	}

	return 0;

drv_err:
	pr_err("cpuidle driver register failed!\n,");
	cpuidle_unregister_driver(&sunxi_idle_driver);

dram_err:
	pr_err("cpuidle dram master clk get failed!\n,");
#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
	unregister_early_suspend(&cpuidle_early_suspend);

notify_err:
#endif

#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG
	unregister_cpu_notifier(&cpuidle_nb);

err_out:
#endif
	return err;
}
device_initcall(sunxi_cpuidle_init);

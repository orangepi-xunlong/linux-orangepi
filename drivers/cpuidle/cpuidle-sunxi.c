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
#include <asm/smp_plat.h>
#include <asm/delay.h>
#include <asm/cp15.h>
#include <asm/suspend.h>
#include <asm/cacheflush.h>
#include <asm/hardware/gic.h>
#include <mach/cpuidle-sunxi.h>
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <mach/sun9i/platsmp.h>

#ifdef CONFIG_ARCH_SUN9IW1P1
#include <linux/clk/clk-sun9iw1.h>
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend cpuidle_early_suspend;
static int cpuidle_early_suspend_flag = 0;
#endif
#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
static struct clk *usb_hci_clk;
static struct clk *usb_otg_clk;
static struct clk *usb_otgphy_clk;
static struct clk *sdmmc_clk0;
static struct clk *sdmmc_clk1;
static struct clk *sdmmc_clk2;
static struct clk *sdmmc_clk3;
static struct clk *nand_clk0;
static struct clk *nand_clk1;
static struct clk *dma_clk;
/* usb */
#define ECHI_PROT_STAT	(0x54)
#define OCHI_PORT_STAT	(0x454)
#define USB_OTG_VBASE	(0xf0900000)
#define	OTG_PORT_STAT	(0x430)
/* sdmmc */
#define SDC_DMAC_STAT	(0x88)
/* dma */
#define DMAC_CHAN_STAT	(0x30)
/* ndfc */
#define NAND_STAT	(0x4)

#define ENCNT(__clkp) (__clkp->enable_count)

static int dram_master_clk_get(void)
{
	usb_hci_clk = clk_get(NULL, USBHCI_CLK);
	if (!usb_hci_clk || IS_ERR(usb_hci_clk)) {
		pr_err("%s:try to get usb_hci_clk failed!\n",__func__);
		return -1;
	}

	usb_otg_clk = clk_get(NULL, USBOTG_CLK);
	if (!usb_otg_clk || IS_ERR(usb_otg_clk)) {
		pr_err("%s:try to get usb_otg_clk failed!\n",__func__);
		return -1 ;
	}

	usb_otgphy_clk = clk_get(NULL, USBOTGPHY_CLK);
	if (!usb_otgphy_clk || IS_ERR(usb_otgphy_clk)) {
		pr_err("%s:try to get usb_otgphy_clk failed!\n",__func__);
		return -1;
	}

	sdmmc_clk0 = clk_get(NULL, SDMMC0_CLK);
	if (!sdmmc_clk0 || IS_ERR(sdmmc_clk0)) {
		pr_err("%s:try to get sdmmc_clk0 failed!\n",__func__);
		return -1;
	}
	sdmmc_clk1 = clk_get(NULL, SDMMC1_CLK);
	if (!sdmmc_clk1 || IS_ERR(sdmmc_clk1)) {
		pr_err("%s:try to get sdmmc_clk1 failed!\n",__func__);
		return -1;
	}
	sdmmc_clk2 = clk_get(NULL, SDMMC2_CLK);
	if (!sdmmc_clk2 || IS_ERR(sdmmc_clk2)) {
		pr_err("%s:try to get sdmmc_clk2 failed!\n",__func__);
		return -1;
	}
	sdmmc_clk3 = clk_get(NULL, SDMMC3_CLK);
	if (!sdmmc_clk3 || IS_ERR(sdmmc_clk3)) {
		pr_err("%s:try to get sdmmc_clk3 failed!\n",__func__);
		return -1;
	}
	nand_clk0 = clk_get(NULL, NAND0_0_CLK);
	if (!nand_clk0 || IS_ERR(nand_clk0)) {
		pr_err("%s:try to get nand_clk0 failed!\n",__func__);
		return -1;
	}
	nand_clk1 = clk_get(NULL, NAND0_1_CLK);
	if (!nand_clk1 || IS_ERR(nand_clk1)) {
		pr_err("%s:try to get nand_clk1 failed!\n",__func__);
		return -1;
	}
	dma_clk = clk_get(NULL, DMA_CLK);
	if (!dma_clk || IS_ERR(dma_clk)) {
		pr_err("%s:try to get dma_clk failed!\n",__func__);
		return -1;
	}
	return 0;
}

static int dram_master_check(void)
{
	unsigned int reg_val=0;
	if(ENCNT(nand_clk0) && ENCNT(nand_clk1)){
		reg_val |= readl(SUNXI_NFC0_VBASE + NAND_STAT) & (0x1<<4);
		if(reg_val)
			return -1;
	}
	if(ENCNT(usb_hci_clk)){
		reg_val |= readl(SUNXI_USB_HCI0_VBASE + ECHI_PROT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI1_VBASE + ECHI_PROT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI2_VBASE + ECHI_PROT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI0_VBASE + OCHI_PORT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI1_VBASE + OCHI_PORT_STAT) & (0x1<<0);
		reg_val |= readl(SUNXI_USB_HCI2_VBASE + OCHI_PORT_STAT) & (0x1<<0);
		if(reg_val)
			return -2;
	}
	if(ENCNT(usb_otg_clk) && ENCNT(usb_otgphy_clk)){
		reg_val |= readl(USB_OTG_VBASE + OTG_PORT_STAT) & (0x1<<17);
		if(reg_val)
			return -3;
	}
	if(ENCNT(sdmmc_clk0))
		reg_val |= readl(SUNXI_SDMMC0_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if(ENCNT(sdmmc_clk1))
		reg_val |= readl(SUNXI_SDMMC1_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if(ENCNT(sdmmc_clk2))
		reg_val |= readl(SUNXI_SDMMC2_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if(ENCNT(sdmmc_clk3))
		reg_val |= readl(SUNXI_SDMMC3_VBASE + SDC_DMAC_STAT) & (0xf<<13);
	if(reg_val)
		return -4;
	if(ENCNT(dma_clk))
		reg_val |= readl(SUNXI_DMA_VBASE + DMAC_CHAN_STAT) & (0x1<<4);
	return reg_val;
}
#endif

#define DRAM_SELFREFLASH		0x1
#define DRAM_NOT_SELFREFLASH		0x0

#ifdef CONFIG_IDLE_FETCH
#include <linux/debugfs.h>
#include <asm/uaccess.h>
static struct dentry *my_idle_root;
static unsigned int cpu0_idle_time_c0 = 0;
static unsigned int cpu0_idle_count_c0 = 0;
static unsigned int cpu0_idle_time_c1 = 0;
static unsigned int cpu0_idle_count_c1 = 0;
static unsigned int cpu0_idle_time_c2 = 0;
static unsigned int cpu0_idle_count_c2 = 0;
static unsigned int cpu0_poweroff_time = 0;
static unsigned int cpu0_poweroff_count = 0;
#endif

/*mask for c1 state*/
struct cpumask sunxi_cpu_try_enter_idle_mask;
struct cpumask sunxi_cpu_idle_mask;
static struct cpumask sunxi_cpu0_try_kill_mask;
static struct cpumask sunxi_cpu0_kill_mask;
DEFINE_RAW_SPINLOCK(sunxi_cpu_idle_c1_lock);

/*mask for c2state*/
static struct cpumask sunxi_core_in_c2state_mask;
DEFINE_RAW_SPINLOCK(sunxi_cpu_idle_c2_lock);

static int cpu0_response_flag = 0;
atomic_t sunxi_user_idle_driver = ATOMIC_INIT(0);
static DEFINE_PER_CPU(struct cpuidle_device, sunxi_cpuidle_device);

static int sunxi_enter_c0state(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
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

	dev->last_residency = idle_time < 0? 0 : idle_time;
#ifdef CONFIG_IDLE_FETCH
	if(dev->cpu == 0){
		cpu0_idle_time_c0 += dev->last_residency;
		cpu0_idle_count_c0++;
	}
#endif
	return index;
}

static void sunxi_cpu0_disable_others(void *info)
{
	int cpu;
	unsigned long flags;
	struct cpumask tmp_mask;

	raw_spin_lock_irqsave(&sunxi_cpu_idle_c1_lock, flags);
	cpu0_response_flag = 0x10;
	while(!cpumask_empty(&sunxi_cpu_try_enter_idle_mask)) {
		for_each_cpu(cpu, &sunxi_cpu_try_enter_idle_mask) {
			/*send an ipi to the target cpu*/
			if(!cpumask_test_cpu(cpu,&sunxi_cpu0_try_kill_mask)) {
				cpumask_set_cpu(cpu, &sunxi_cpu0_try_kill_mask);
				cpumask_clear(&tmp_mask);
				cpumask_set_cpu(cpu, &tmp_mask);
				gic_raise_softirq(&tmp_mask, 0);
				__delay(50);
			}
			if(!cpumask_test_cpu(cpu,&sunxi_cpu0_kill_mask)){
				/*target cpu maybe two state,in wfi or wait for lock*/
				raw_spin_unlock(&sunxi_cpu_idle_c1_lock);
			   /*
				* here need delay to let other cpu get lock
				*/
				__delay(50);
			   /*
				* give a chance for others to clear
				* sunxi_cpu_try_enter_idle_mask
				*/
				raw_spin_lock(&sunxi_cpu_idle_c1_lock);
				continue;
			}
			/* if current cpu is not in wfi, try next */
			while(!SUNXI_CPU_IS_WFI_MODE(cpu)) {
				continue;
			}
			sunxi_cpuidle_power_down_cpu(cpu);
			cpumask_set_cpu(cpu, &sunxi_cpu_idle_mask);
			cpumask_clear_cpu(cpu, &sunxi_cpu0_kill_mask);
			cpumask_clear_cpu(cpu, &sunxi_cpu0_try_kill_mask);
            cpumask_clear_cpu(cpu, &sunxi_cpu_try_enter_idle_mask);
		}
	}

	cpu0_response_flag = 0;
   /*
	* here clear sunxi_cpu_kill_mask because
	* may be cpu0 send a wake up ipi to cpux,
	* but a cpu is waiting a lock to leave idle state and no change to be powered down.
	*/
	cpumask_clear(&sunxi_cpu0_try_kill_mask);
	raw_spin_unlock_irqrestore(&sunxi_cpu_idle_c1_lock, flags);
}

static int sunxi_sleep_cpu_secondary_finish(unsigned long val)
{
	unsigned long flags;
	bool need_smp_call;
	unsigned int cpu;

    /* check if need response some ipi interrupt */
	if(sunxi_pending_sgi()){
		return 1;
	}else{
		raw_spin_lock_irqsave(&sunxi_cpu_idle_c1_lock, flags);
		cpu = get_logical_index(read_cpuid_mpidr()&0xFFFF);
		need_smp_call = cpumask_empty(&sunxi_cpu_try_enter_idle_mask);
		cpumask_set_cpu(cpu, &sunxi_cpu_try_enter_idle_mask);

		if(need_smp_call && !cpu0_response_flag) {
			cpu0_response_flag = 0x01;
			raw_spin_unlock_irqrestore(&sunxi_cpu_idle_c1_lock, flags);
			smp_call_function_single(0,sunxi_cpu0_disable_others,0,0);
		}else{
			raw_spin_unlock_irqrestore(&sunxi_cpu_idle_c1_lock, flags);
		}
		asm("wfi");

		cpu = get_logical_index(read_cpuid_mpidr()&0xFFFF);
		if(cpumask_test_cpu(cpu,&sunxi_cpu0_try_kill_mask)) {
			cpumask_set_cpu(cpu,&sunxi_cpu0_kill_mask);
			sunxi_idle_cpu_die();
		}
		raw_spin_lock_irqsave(&sunxi_cpu_idle_c1_lock, flags);
		cpumask_clear_cpu(cpu, &sunxi_cpu_try_enter_idle_mask);
		raw_spin_unlock_irqrestore(&sunxi_cpu_idle_c1_lock, flags);

		return 1;
	}
}

static int sunxi_cpu_core_power_down(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	int ret;
	cpu_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();
	ret = cpu_suspend(0, sunxi_sleep_cpu_secondary_finish);
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	cpu_pm_exit();

	return index;
}
static int sunxi_enter_c1state(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	if(dev->cpu == 0){
		cpu_do_idle();
	}else{
		sunxi_cpu_core_power_down(dev,drv,index);
	}

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
				(after.tv_usec - before.tv_usec);
	dev->last_residency = idle_time;
#ifdef CONFIG_IDLE_FETCH
	if(dev->cpu == 0){
		cpu0_idle_time_c1 += dev->last_residency;
		cpu0_idle_count_c1++;
	}
#endif

	return index;
}

static int sunxi_sleep_all_core_finish(unsigned long val)
{
	unsigned int cpu;
	struct cpumask tmp;
	struct sunxi_cpuidle_para sunxi_enter_idle_para_info;
	cpu = get_logical_index(read_cpuid_mpidr()&0xFFFF);

	raw_spin_lock(&sunxi_cpu_idle_c2_lock);
	cpumask_copy(&tmp,&sunxi_core_in_c2state_mask);

	if(cpumask_equal(&tmp,&cpu_power_up_state_mask)){
		cpumask_clear_cpu(cpu,&tmp);
		if(cpumask_equal(&tmp, &sunxi_cpu_idle_mask) && (!gic_pending_irq(0))){

			raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
			/* call cpus interface to power down cpu0*/
			if(!gic_pending_irq(0)){

				/*set cpu0 entry address*/
				mcpm_set_entry_vector(0, 0, cpu_resume);
				/*call cpus interface to clear its irq pending*/

#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
				if(cpuidle_early_suspend_flag
						&& !dram_master_check()){
					sunxi_enter_idle_para_info.flags = DRAM_SELFREFLASH;
				}else
#endif
					sunxi_enter_idle_para_info.flags = DRAM_NOT_SELFREFLASH;
				arisc_enter_cpuidle(NULL,NULL,(struct sunxi_cpuidle_para *)(&sunxi_enter_idle_para_info));
				sunxi_idle_cluster_die(A7_CLUSTER);
			}else{
				asm("wfi");
			}
		}else{
			raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
		}
	}else{
		raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
		asm("wfi");
	}

	return 1;
}

static int sunxi_all_cpu_power_down_in_c2state(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	int ret;
	cpu_pm_enter();
	//cpu_cluster_pm_enter();
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);
	smp_wmb();
	ret = cpu_suspend(0, sunxi_sleep_all_core_finish);
	clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);
	//cpu_cluster_pm_exit();
	cpu_pm_exit();
	return index;
}
static int sunxi_enter_c2state(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
				int index)
{
	struct timeval before, after;
	int idle_time;
	unsigned int cpu_id;
#ifdef CONFIG_IDLE_FETCH
	int power_off_flag = 0;
#endif

	local_irq_disable();
	do_gettimeofday(&before);
	cpu_id = get_logical_index(read_cpuid_mpidr()&0xFFFF);

	if(dev->cpu >= MAX_CPU_IN_CLUSTER){
		/*other cpu in c3state*/
		sunxi_cpu_core_power_down(dev,drv,index);
	}else{

		raw_spin_lock(&sunxi_cpu_idle_c2_lock);
		cpumask_set_cpu(cpu_id,&sunxi_core_in_c2state_mask);
		if(dev->cpu == 0){
			if(cpumask_equal(&sunxi_core_in_c2state_mask, &cpu_power_up_state_mask)){
				raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
#ifdef CONFIG_IDLE_FETCH
				power_off_flag = 1;
#endif
				sunxi_all_cpu_power_down_in_c2state(dev,drv,index);
			}else{
				raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
				cpu_do_idle();
			}
		}else{
			raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
			sunxi_cpu_core_power_down(dev,drv,index);
		}
		raw_spin_lock(&sunxi_cpu_idle_c2_lock);
		cpumask_clear_cpu(cpu_id,&sunxi_core_in_c2state_mask);
		raw_spin_unlock(&sunxi_cpu_idle_c2_lock);
	}
	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
				(after.tv_usec - before.tv_usec);

	dev->last_residency = idle_time < 0? 0 : idle_time;
#ifdef CONFIG_IDLE_FETCH
	if(dev->cpu == 0){
		cpu0_idle_time_c2 += dev->last_residency;
		cpu0_idle_count_c2++;
	}
	if(power_off_flag){
		cpu0_poweroff_time += dev->last_residency;
		cpu0_poweroff_count++;
	}
#endif
	return index;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sunxi_cpuidle_earlysuspend(struct early_suspend *h)
{
	pr_debug("%s:enter\n",__func__);
	cpuidle_early_suspend_flag = 1;
}

static void sunxi_cpuidle_lateresume(struct early_suspend *h)
{
	pr_debug("%s:resume\n",__func__);
	cpuidle_early_suspend_flag = 0;
}
#endif

/**
 * @enter: low power process function
 * @exit_latency: latency of exit this state, based on us
 * @power_usage: power used by cpu under this state, based on mw
 * @target_residency: the minimum of time should cpu spend in
 *   this state, based on us
 */
static struct cpuidle_state sunxi_cpuidle_set[] __initdata = {
	[0] = {
		.enter			    = sunxi_enter_c0state,
		.exit_latency		= 1,
		.power_usage        = 1000,
		.target_residency	= 100,
		.flags			    = CPUIDLE_FLAG_TIME_VALID,
		.name			    = "C0",
		.desc			    = "ARM clock gating(WFI)",
	},
	[1] = {
		.enter			    = sunxi_enter_c1state,
		.exit_latency		= 3000,
		.power_usage        = 500,
		.target_residency	= 10000,
		.flags			    = CPUIDLE_FLAG_TIME_VALID,
		.name			    = "C1",
		.desc			    = "SUNXI CORE POWER DOWN",
	},
#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG
	[2] = {
		.enter              = sunxi_enter_c2state,
		.exit_latency       = 10000,
		.power_usage        = 100,
		.target_residency   = 20000,
		.flags              = CPUIDLE_FLAG_TIME_VALID,
		.name               = "C2",
		.desc               = "SUNXI CLUSTER POWER DOWN",
	},
#endif
};

static struct cpuidle_driver sunxi_idle_driver = {
	.name		= "sunxi_idle",
	.owner		= THIS_MODULE,
};
#ifdef CONFIG_IDLE_FETCH
static ssize_t sunxi_read_idle(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	char temp[128];
	int len;
	sprintf(temp, "\nc0 :%u count:%u\nc1 :%u count:%u\nc2 :%u count:%u\noff:%u count:%u\n",
		cpu0_idle_time_c0,cpu0_idle_count_c0,cpu0_idle_time_c1,cpu0_idle_count_c1,
		cpu0_idle_time_c2,cpu0_idle_count_c2,cpu0_poweroff_time,cpu0_poweroff_count);
	len = strlen(temp);
	if(len){
		if(*ppos >=len)
			return 0;
		if(count >=len)
			count = len;
		if(count > (len - *ppos))
			count = (len - *ppos);
		copy_to_user((void __user *)buf,(const void *)temp,(unsigned long)len);
		*ppos += count;
	}
	else
		count = 0;
	return count;
}

static const struct file_operations idle_ops = {
    .read        = sunxi_read_idle,
};
#endif

static int __init sunxi_init_cpuidle(void)
{
	int i, max_cpuidle_state, cpu;
	struct cpuidle_device *device;

	sunxi_set_bootcpu_hotplugflg();
	sunxi_set_secondary_entry((void *)(virt_to_phys(mcpm_entry_point)));

	cpumask_clear(&sunxi_cpu_try_enter_idle_mask);
	cpumask_clear(&sunxi_cpu_idle_mask);
	cpumask_clear(&sunxi_cpu_try_enter_idle_mask);
	cpumask_clear(&sunxi_cpu0_kill_mask);
	atomic_set(&sunxi_user_idle_driver,0x01);

	max_cpuidle_state = ARRAY_SIZE(sunxi_cpuidle_set);
	sunxi_idle_driver.state_count = max_cpuidle_state;

	for (i = 0; i < max_cpuidle_state; i++) {
		sunxi_idle_driver.states[i] = sunxi_cpuidle_set[i];
	}

	sunxi_idle_driver.safe_state_index = 0;
	cpuidle_register_driver(&sunxi_idle_driver);

	for_each_possible_cpu(cpu) {
		device = &per_cpu(sunxi_cpuidle_device, cpu);
		device->cpu = cpu;
		device->state_count = max_cpuidle_state;
		if (cpuidle_register_device(device)) {
			printk(KERN_ERR "CPUidle register device failed\n,");
			return -EIO;
		}
	}
#ifdef CONFIG_DRAM_SELFREFLASH_IN_CPUIDLE
	if(dram_master_clk_get()){
			printk(KERN_ERR "CPUidle get module clk failed\n,");
			return -EIO;
		}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	cpuidle_early_suspend.suspend = sunxi_cpuidle_earlysuspend;
	cpuidle_early_suspend.resume = sunxi_cpuidle_lateresume;
	cpuidle_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 300;
	register_early_suspend(&cpuidle_early_suspend);
#endif
#ifdef CONFIG_IDLE_FETCH
		my_idle_root = debugfs_create_dir("idle_fetch", NULL);
		debugfs_create_file("fetch", 0644, my_idle_root, NULL,&idle_ops);
#endif
	return 0;
}
device_initcall(sunxi_init_cpuidle);

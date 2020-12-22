/*
 * drivers/devfreq/dramfreq/sunxi-dramfreq.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUNXI dram frequency dynamic scaling driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/devfreq.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#include <linux/clk/sunxi_name.h>
#include <linux/wakelock.h>
#include <mach/sys_config.h>
#include "sunxi-dramfreq.h"

#undef DRAMFREQ_DBG
#undef DRAMFREQ_ERR
#if (1)
    #define DRAMFREQ_DBG(format,args...)   printk("[dramfreq] "format,##args)
#else
    #define DRAMFREQ_DBG(format,args...)   do{}while(0)
#endif
#define DRAMFREQ_ERR(format,args...)   printk(KERN_ERR "[dramfreq] ERR:"format,##args)

#define SRAM_MDFS_START (0xf0000000)

#if defined(CONFIG_ARCH_SUN8IW3P1) && defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_DEVFREQ_GOV_VANS)
extern struct timer_list dramfreq_timer;
extern unsigned long dramfreq_expired_time;
#endif

extern char *mdfs_bin_start;
extern char *mdfs_bin_end;

static struct clk *clk_pll5; /* pll5 clock handler */
static struct clk *ahb1;     /* ahb1 clock handler */
__dram_para_t dramfreq_info;
static spinlock_t mdfs_spin_lock;
static struct devfreq *this_df = NULL;
static unsigned long long setfreq_time_usecs = 0;
static unsigned long long getfreq_time_usecs = 0;
static DEFINE_MUTEX(ahb_update_lock);
static struct wake_lock dramfreq_wake_lock;

#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
struct master_bw_info master_info_list[] = {
    { MASTER_CPU    , "CPU"     },
    { MASTER_GPU    , "GPU"     },
    { MASTER_VE     , "VE"      },
    { MASTER_DISPLAY, "DISPLAY" },
    { MASTER_OTHER  , "OTHER"   },
    { MASTER_TOTAL  , "TOTAL"   },
};

static unsigned int dramfreq_frequency_table[] = {
    644000,
    552000,
    297000,
    161000,
    DRAMFREQ_TABLE_END,
};
#endif

static int bit_wide = 0;
unsigned int master_bw_usage[MASTER_MAX] = {0};
unsigned int bw_history[MASTER_MAX] = {0};
#endif

#ifdef CONFIG_DRAM_FREQ_DVFS
#define TABLE_LENGTH (8)
static unsigned int table_length_syscfg = 0;
static unsigned int last_vdd = 1240; /* backup last target voltage, default is 1.24v */
static struct regulator *dcdc2 = NULL;
struct dramfreq_dvfs {
    unsigned int    freq;   /* cpu frequency, based on KHz */
    unsigned int    volt;   /* voltage for the frequency, based on mv */
};
static struct dramfreq_dvfs dvfs_table_syscfg[TABLE_LENGTH];

static inline unsigned int dramfreq_vdd_value(unsigned int freq)
{
    struct dramfreq_dvfs *dvfs_inf = NULL;
    dvfs_inf = &dvfs_table_syscfg[0];

    while((dvfs_inf+1)->freq >= freq)
        dvfs_inf++;

    return dvfs_inf->volt;
}

static int __init_vftable_syscfg(void)
{
    int i, ret = 0;
    char name[16] = {0};
    script_item_u val;
    script_item_value_type_e type;

    type = script_get_item("dram_dvfs_table", "LV_count", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch LV_count from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("LV_count value is %x\n", val.val);
    table_length_syscfg = val.val;

    /* table_length_syscfg must be < TABLE_LENGTH */
    if(table_length_syscfg > TABLE_LENGTH){
        DRAMFREQ_ERR("LV_count from sysconfig is out of bounder\n");
        ret = -1;
        goto fail;
    }

    for (i = 1; i <= table_length_syscfg; i++){
        sprintf(name, "LV%d_freq", i);
        type = script_get_item("dram_dvfs_table", name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
            DRAMFREQ_ERR("get LV%d_freq from sysconfig failed\n", i);
            return -ENODEV;
        }
        dvfs_table_syscfg[i-1].freq = val.val / 1000;

        sprintf(name, "LV%d_volt", i);
        type = script_get_item("dram_dvfs_table", name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
            DRAMFREQ_ERR("get LV%d_volt from sysconfig failed\n", i);
            return -ENODEV;
        }
        dvfs_table_syscfg[i-1].volt = val.val;
    }

fail:
    return ret;
}

static void __vftable_show(void)
{
    int i;

    DRAMFREQ_DBG("-------------------Dram V-F Table--------------------\n");
    for(i = 0; i < table_length_syscfg; i++){
        DRAMFREQ_DBG("\tvoltage = %4dmv \tfrequency = %4dKHz\n",
            dvfs_table_syscfg[i].volt, dvfs_table_syscfg[i].freq);
    }
    DRAMFREQ_DBG("-----------------------------------------------------\n");
}
#endif

static unsigned long __dramfreq_get(struct clk *pll5)
{
    unsigned long pll5_rate, dram_freq;

    pll5_rate = clk_get_rate(pll5) / 1000;

    if(readl(SUNXI_CCM_VBASE + 0x020) & 0x3)    //pll normal mode
        dram_freq = pll5_rate * 2;
    else                                        //pll bypass mode
        dram_freq = pll5_rate / 4;

    return dram_freq;
}

/**
 * dramfreq_get - get the current DRAM frequency (in KHz)
 *
 */
unsigned long dramfreq_get(void)
{
    unsigned long freq;
    ktime_t calltime = ktime_set(0, 0), delta, rettime;

    calltime = ktime_get();

    freq = __dramfreq_get(clk_pll5);

    rettime = ktime_get();
    delta = ktime_sub(rettime, calltime);
    getfreq_time_usecs = ktime_to_ns(delta) >> 10;

    return freq;
}
EXPORT_SYMBOL_GPL(dramfreq_get);

int __ahb_set_rate(unsigned long ahb_freq)
{
    mutex_lock(&ahb_update_lock);

    if (clk_prepare_enable(ahb1)) {
        DRAMFREQ_ERR("try to enable ahb1 output failed!\n");
        goto err;
    }

    if (clk_get_rate(ahb1) == ahb_freq) {
        mutex_unlock(&ahb_update_lock);
        return 0;
    }

    if (clk_set_rate(ahb1, ahb_freq)) {
        DRAMFREQ_ERR("try to set ahb1 rate to %lu failed!\n", ahb_freq);
        goto err;
    }

    mutex_unlock(&ahb_update_lock);
    return 0;

err:
    mutex_unlock(&ahb_update_lock);
    return -1;
}

/**
 *  freq_target: target frequency
 *  df: devfreq
 */
static int __dramfreq_set(struct devfreq *df, unsigned int freq_target)
{
    unsigned long flags;
    ktime_t calltime = ktime_set(0, 0), delta, rettime;
#if defined(CONFIG_ARCH_SUN8IW3P1)
    int (*mdfs_main)(__dram_para_t *para);
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
    int (*mdfs_main)(int type, int freq_jump, __dram_para_t *para);
    unsigned int tmp_freq_jump, tmp_type = 1;
#endif

#if defined(CONFIG_ARCH_SUN8IW3P1)
    mdfs_main = (int (*)(__dram_para_t *para))SRAM_MDFS_START;
    dramfreq_info.dram_clk = freq_target / 1000;
    dramfreq_info.high_freq = df->max_freq / 1000;
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
    mdfs_main = (int (*)(int type, int freq_jump, __dram_para_t *para))SRAM_MDFS_START;
    tmp_freq_jump = freq_target < df->previous_freq ? 0 : 1;
#endif

    /* move code to sram */
    memcpy((void *)SRAM_MDFS_START, (void *)&mdfs_bin_start, (int)&mdfs_bin_end - (int)&mdfs_bin_start);

    spin_lock_irqsave(&mdfs_spin_lock, flags);
    calltime = ktime_get();

#if defined(CONFIG_ARCH_SUN8IW3P1)
    mdfs_main(&dramfreq_info);
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
    mdfs_main(tmp_type, tmp_freq_jump, &dramfreq_info);
#endif

    rettime = ktime_get();
    delta = ktime_sub(rettime, calltime);
    setfreq_time_usecs = ktime_to_ns(delta) >> 10;
    spin_unlock_irqrestore(&mdfs_spin_lock, flags);

    DRAMFREQ_DBG("[switch time]: %Ld usecs\n", setfreq_time_usecs);
    DRAMFREQ_DBG("dram: %luKHz->%luKHz ok!\n", df->previous_freq, __dramfreq_get(clk_pll5));

    return 0;
}

#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
int __get_valid_freq(unsigned long freq)
{
    unsigned int *target = &dramfreq_frequency_table[0];

    while (*target != DRAMFREQ_TABLE_END) {
        if(*(target + 1) >= freq)
            target++;
        else
            break;
    }

    return *target;
}
#endif

static int dramfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
    struct platform_device *pdev = container_of(dev, struct platform_device, dev);
    struct devfreq *df = platform_get_drvdata(pdev);
#ifdef CONFIG_DRAM_FREQ_DVFS
    int ret = 0;
    unsigned int new_vdd;
#endif

#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
    *freq = __get_valid_freq(*freq);
#endif

    if (*freq == df->previous_freq) {
        DRAMFREQ_DBG("freq_calc == df->previous_freq\n");
        return 0;
    }

    DRAMFREQ_DBG("want to set dram frequency from %luKHz to %luKHz\n", df->previous_freq, *freq);

#ifdef CONFIG_ARCH_SUN8IW3P1
    if (*freq > df->previous_freq) {
        if (!__ahb_set_rate(200000000))
            DRAMFREQ_DBG("set ahb to 200MHz ok!\n");
    }
#endif

#ifdef CONFIG_DRAM_FREQ_DVFS
    dcdc2 = regulator_get(NULL, "axp22_dcdc2");
    if (IS_ERR(dcdc2)) {
        DRAMFREQ_ERR("some error happen, fail to get regulator!");
        dcdc2 = NULL;
    }

    new_vdd = dramfreq_vdd_value(*freq);
    if (dcdc2 && (new_vdd > last_vdd)) {
        ret = regulator_set_voltage(dcdc2, new_vdd*1000, new_vdd*1000);
        if(ret < 0) {
            DRAMFREQ_ERR("fail to set regulator voltage!\n");
            regulator_put(dcdc2);
            return ret;
        }
        DRAMFREQ_DBG("dcdc2: %dmv->%dmv ok!\n", last_vdd, new_vdd);
    }
#endif

    wake_lock(&dramfreq_wake_lock);
    __dramfreq_set(df, *freq);
    wake_unlock(&dramfreq_wake_lock);

#ifdef CONFIG_DRAM_FREQ_DVFS
    if (dcdc2 && (new_vdd < last_vdd)) {
        ret = regulator_set_voltage(dcdc2, new_vdd*1000, new_vdd*1000);
        if (ret < 0) {
            DRAMFREQ_ERR("fail to set regulator voltage!\n");
            new_vdd = last_vdd;
            regulator_put(dcdc2);
            return ret;
        }
        DRAMFREQ_DBG("dcdc2: %dmv->%dmv ok!\n", last_vdd, new_vdd);
    }
    last_vdd = new_vdd;
#endif

#ifdef CONFIG_ARCH_SUN8IW3P1
    if (*freq < df->previous_freq) {
        if (!__ahb_set_rate(500000))
            DRAMFREQ_DBG("set ahb to 50MHz ok!\n");
    }
#endif

    *freq = __dramfreq_get(clk_pll5);

#ifdef CONFIG_DRAM_FREQ_DVFS
    if (dcdc2) {
        regulator_put(dcdc2);
        dcdc2 = NULL;
    }
#endif

    return 0;
}

static int dramfreq_get_dev_status(struct device *dev,
                        struct devfreq_dev_status *stat)
{
#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
    unsigned int i, bw_cur, bw_usage;
    enum master_bw_type mbt;

    for (i = 0; i < ARRAY_SIZE(master_info_list); i++) {
        mbt = master_info_list[i].type;
        bw_cur = readl(SDR_COM_BWCR(i));
        bw_usage = bw_cur < bw_history[mbt] ? \
            bw_cur + (UINT_MAX - bw_history[mbt]) : \
            bw_cur - bw_history[mbt];
        master_bw_usage[mbt] = bw_usage;
        bw_history[mbt] = bw_cur;
    }

    stat->current_frequency = __dramfreq_get(clk_pll5);
    stat->private_data = &bit_wide;
#endif

    return 0;
}

static struct devfreq_dev_profile dram_devfreq_profile = {
    .target         = dramfreq_target,
    .get_dev_status = dramfreq_get_dev_status,
#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
    .polling_ms     = DRAMFREQ_POLLING_MS,
#endif
};

static ssize_t mdfs_table_show(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    int m, n;

    for (m = 0; m < DRAM_MDFS_TABLE_PARA0; m++) {
        for (n = 0; n < DRAM_MDFS_TABLE_PARA1; n++) {
            DRAMFREQ_DBG("dramfreq_table[%d][%d]=0x%x\n", m, n, dramfreq_info.table[m][n]);
        }
    }

    return 0;
}

static DEVICE_ATTR(mdfs_table, S_IRUGO, mdfs_table_show, NULL);

static const struct attribute *dramfreq_attrib[] = {
    &dev_attr_mdfs_table.attr,
    NULL
};

static __devinit int sunxi_dramfreq_probe(struct platform_device *pdev)
{
    void *tmp_tbl_soft = NULL;
    int err = 0;
    script_item_u val;
    script_item_value_type_e type;
    unsigned int sunxi_dramfreq_min = 0, sunxi_dramfreq_max = 0;

    type = script_get_item("dram_para", "dram_clk", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_clk from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_clk value is %x\n", val.val);
    dramfreq_info.dram_clk = val.val;
    sunxi_dramfreq_max = dramfreq_info.dram_clk * 1000;
    DRAMFREQ_DBG("sunxi_dramfreq_max=%u\n", sunxi_dramfreq_max);

    type = script_get_item("dram_para", "dram_type", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_type from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_type value is %x\n", val.val);
    dramfreq_info.dram_type = val.val;

    type = script_get_item("dram_para", "dram_zq", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_zq from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_zq value is %x\n", val.val);
    dramfreq_info.dram_zq = val.val;

    type = script_get_item("dram_para", "dram_odt_en", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_odt_en from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_odt_en value is %x\n", val.val);
    dramfreq_info.dram_odt_en = val.val;

    type = script_get_item("dram_para", "dram_para1", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_para1 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_para1 value is %x\n", val.val);
    dramfreq_info.dram_para1 = val.val;

    type = script_get_item("dram_para", "dram_para2", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_para2 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_para2 value is %x\n", val.val);
    dramfreq_info.dram_para2 = val.val;

    type = script_get_item("dram_para", "dram_mr0", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_mr0 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_mr0 value is %x\n", val.val);
    dramfreq_info.dram_mr0 = val.val;

    type = script_get_item("dram_para", "dram_mr1", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_mr1 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_mr1 value is %x\n", val.val);
    dramfreq_info.dram_mr1 = val.val;

    type = script_get_item("dram_para", "dram_mr2", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_mr2 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_mr2 value is %x\n", val.val);
    dramfreq_info.dram_mr2 = val.val;

    type = script_get_item("dram_para", "dram_mr3", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_mr3 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_mr3 value is %x\n", val.val);
    dramfreq_info.dram_mr3 = val.val;

    type = script_get_item("dram_para", "dram_tpr0", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr0 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr0 value is %x\n", val.val);
    dramfreq_info.dram_tpr0 = val.val;

    type = script_get_item("dram_para", "dram_tpr1", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr1 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr1 value is %x\n", val.val);
    dramfreq_info.dram_tpr1 = val.val;

    type = script_get_item("dram_para", "dram_tpr2", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr2 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr2 value is %x\n", val.val);
    dramfreq_info.dram_tpr2 = val.val;

    type = script_get_item("dram_para", "dram_tpr3", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr3 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr3 value is %x\n", val.val);
    dramfreq_info.dram_tpr3 = val.val;

    type = script_get_item("dram_para", "dram_tpr4", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr4 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr4 value is %x\n", val.val);
    dramfreq_info.dram_tpr4 = val.val;

    type = script_get_item("dram_para", "dram_tpr5", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr5 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr5 value is %x\n", val.val);
    dramfreq_info.dram_tpr5 = val.val;

    type = script_get_item("dram_para", "dram_tpr6", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr6 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr6 value is %x\n", val.val);
    dramfreq_info.dram_tpr6 = val.val;

    type = script_get_item("dram_para", "dram_tpr7", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr7 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr7 value is %x\n", val.val);
    dramfreq_info.dram_tpr7 = val.val;

    type = script_get_item("dram_para", "dram_tpr8", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr8 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr8 value is %x\n", val.val);
    dramfreq_info.dram_tpr8 = val.val;

    type = script_get_item("dram_para", "dram_tpr9", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr9 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr9 value is %x\n", val.val);
    dramfreq_info.dram_tpr9 = val.val;

    type = script_get_item("dram_para", "dram_tpr10", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr10 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr10 value is %x\n", val.val);
    dramfreq_info.dram_tpr10 = val.val;

    type = script_get_item("dram_para", "dram_tpr11", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr11 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr11 value is %x\n", val.val);
    dramfreq_info.dram_tpr11 = val.val;

    type = script_get_item("dram_para", "dram_tpr12", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr12 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr12 value is %x\n", val.val);
    dramfreq_info.dram_tpr12 = val.val;
    sunxi_dramfreq_min = dramfreq_info.dram_tpr12 * 1000;
    DRAMFREQ_DBG("sunxi_dramfreq_min=%u\n", sunxi_dramfreq_min);

    type = script_get_item("dram_para", "dram_tpr13", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        DRAMFREQ_ERR("fetch dram_tpr13 from sysconfig failed\n");
        return -ENODEV;
    }
    DRAMFREQ_DBG("dram_tpr13 value is %x\n", val.val);
    dramfreq_info.dram_tpr13 = val.val;

    tmp_tbl_soft = __va(SYS_CONFIG_MEMBASE + SYS_CONFIG_MEMSIZE - 1024);
    memcpy(dramfreq_info.table, tmp_tbl_soft, sizeof(dramfreq_info.table));

#ifdef CONFIG_DEVFREQ_GOV_VANS_POLLING
    /* enable bw counter */
    writel(0x1, SDR_COM_MCGCR);
    bit_wide = 8;
#endif

#if defined(CONFIG_ARCH_SUN8IW3P1)
    clk_pll5 = clk_get(NULL, PLL5_CLK);
#elif defined(CONFIG_ARCH_SUN8IW5P1)
    clk_pll5 = clk_get(NULL, PLL_DDR0_CLK);
#elif defined(CONFIG_ARCH_SUN8IW6P1)
    clk_pll5 = clk_get(NULL, PLL_DDR_CLK);
#endif
    if (!clk_pll5 || IS_ERR(clk_pll5)) {
        DRAMFREQ_ERR("try to get PLL5 failed!\n");
        err = -ENOENT;
        goto err_pll5;
    }

    ahb1 = clk_get(NULL, AHB1_CLK);
    if (!ahb1 || IS_ERR(ahb1)) {
        DRAMFREQ_ERR("try to get AHB1 failed!\n");
        err = -ENOENT;
        goto err_ahb1;
    }

    dram_devfreq_profile.initial_freq = __dramfreq_get(clk_pll5);
#if defined(CONFIG_FPGA_V4_PLATFORM) || defined(CONFIG_FPGA_V7_PLATFORM) || defined(CONFIG_DRAM_FREQ_BSP_TEST)
    this_df = devfreq_add_device(&pdev->dev, &dram_devfreq_profile, &devfreq_userspace, NULL);
#elif defined(CONFIG_EVB_PLATFORM)
    this_df = devfreq_add_device(&pdev->dev, &dram_devfreq_profile, &devfreq_vans, NULL);
#endif
    if (IS_ERR(this_df)) {
        DRAMFREQ_ERR("add devfreq device failed!\n");
        err = PTR_ERR(this_df);
        goto err_devfreq;
    }

    this_df->min_freq = this_df->scaling_min_freq = sunxi_dramfreq_min;
    this_df->max_freq = this_df->scaling_max_freq = sunxi_dramfreq_max;
    platform_set_drvdata(pdev, this_df);

    err = sysfs_create_files(&pdev->dev.kobj, dramfreq_attrib);
    if (err) {
        DRAMFREQ_ERR("create sysfs file failed\n");
        goto err_create_files;
    }

#ifdef CONFIG_DRAM_FREQ_DVFS
    err = __init_vftable_syscfg();
    if(err) {
        DRAMFREQ_ERR("init V-F Table failed\n");
        goto err_create_files;
    }

    __vftable_show();

    dcdc2 = regulator_get(NULL, "axp22_dcdc2");
    if (IS_ERR(dcdc2)) {
        DRAMFREQ_ERR("some error happen, fail to get regulator!");
        goto err_create_files;
    } else {
        last_vdd = regulator_get_voltage(dcdc2) / 1000;
        DRAMFREQ_DBG("last_vdd=%d\n", last_vdd);
    }

    if (dcdc2) {
        regulator_put(dcdc2);
        dcdc2 = NULL;
    }
#endif

    wake_lock_init(&dramfreq_wake_lock, WAKE_LOCK_SUSPEND, "dramfreq_wakelock");

    DRAMFREQ_DBG("sunxi dramfreq probe ok!\n");

    return 0;

err_create_files:
    devfreq_remove_device(this_df);
err_devfreq:
    clk_put(ahb1);
    ahb1 = NULL;
err_ahb1:
    clk_put(clk_pll5);
    clk_pll5 = NULL;
err_pll5:
    return err;
}

static __devexit int sunxi_dramfreq_remove(struct platform_device *pdev)
{
    struct devfreq *df = platform_get_drvdata(pdev);

    devfreq_remove_device(df);

    if (!ahb1 || IS_ERR(ahb1)) {
        DRAMFREQ_ERR("ahb1 handle is invalid, just return!\n");
        return -EINVAL;
    } else {
        clk_put(ahb1);
        ahb1 = NULL;
    }

    if (!clk_pll5 || IS_ERR(clk_pll5)) {
        DRAMFREQ_ERR("clk_pll5 handle is invalid, just return!\n");
        return -EINVAL;
    } else {
        clk_put(clk_pll5);
        clk_pll5 = NULL;
    }

    wake_lock_destroy(&dramfreq_wake_lock);

    return 0;
}

#ifdef CONFIG_PM
static int sunxi_dramfreq_suspend(struct platform_device *pdev, pm_message_t state)
{
#if defined(CONFIG_ARCH_SUN8IW3P1) && defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_DEVFREQ_GOV_VANS)
    struct devfreq *df = platform_get_drvdata(pdev);
    unsigned long target = df->max_freq;

    if (time_before(jiffies, dramfreq_expired_time)) {
        DRAMFREQ_DBG("%s: timer pending!\n", __func__);
        del_timer_sync(&dramfreq_timer);
    }

    if (!dramfreq_target(&pdev->dev, &target, 0))
        df->previous_freq = target;
#endif

    DRAMFREQ_DBG("%s: done!\n", __func__);
    return 0;
}
#endif

static struct platform_driver sunxi_dramfreq_driver = {
    .probe  = sunxi_dramfreq_probe,
    .remove = sunxi_dramfreq_remove,
#ifdef CONFIG_PM
    .suspend = sunxi_dramfreq_suspend,
#endif
    .driver = {
        .name   = "sunxi-dramfreq",
        .owner  = THIS_MODULE,
    },
};

struct platform_device sunxi_dramfreq_device = {
    .name   = "sunxi-dramfreq",
    .id     = -1,
};

static int __init sunxi_dramfreq_init(void)
{
    int ret = 0;

    ret = platform_device_register(&sunxi_dramfreq_device);
    if (ret) {
        DRAMFREQ_ERR("dramfreq device init failed!\n");
        goto out;
    }

    ret = platform_driver_register(&sunxi_dramfreq_driver);
    if (ret) {
        DRAMFREQ_ERR("dramfreq driver init failed!\n");
        goto out;
    }

out:
    return ret;
}
fs_initcall(sunxi_dramfreq_init);

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_dramfreq_root;

static int set_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", setfreq_time_usecs);
    return 0;
}

static int set_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, set_time_show, inode->i_private);
}

static const struct file_operations set_time_fops = {
    .open = set_time_open,
    .read = seq_read,
};

static int get_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", getfreq_time_usecs);
    return 0;
}

static int get_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, get_time_show, inode->i_private);
}

static const struct file_operations get_time_fops = {
    .open = get_time_open,
    .read = seq_read,
};

static int __init debug_init(void)
{
    int err = 0;

    debugfs_dramfreq_root = debugfs_create_dir("dramfreq", 0);
    if (!debugfs_dramfreq_root)
        return -ENOMEM;

    if (!debugfs_create_file("freq_get_time", 0444, debugfs_dramfreq_root, NULL, &get_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    if (!debugfs_create_file("freq_set_time", 0444, debugfs_dramfreq_root, NULL, &set_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    return 0;

out:
    debugfs_remove_recursive(debugfs_dramfreq_root);
    return err;
}

static void __exit debug_exit(void)
{
    debugfs_remove_recursive(debugfs_dramfreq_root);
}

late_initcall(debug_init);
module_exit(debug_exit);
#endif /* CONFIG_DEBUG_FS */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUNXI dramfreq driver with devfreq framework");
MODULE_AUTHOR("pannan");

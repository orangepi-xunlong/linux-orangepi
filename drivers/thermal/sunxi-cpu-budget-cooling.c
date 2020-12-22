/*
 * drivers/thermal/sunxi-cpu-cooling.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *	kevin.z.m<kevin@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/cpu_budget_cooling.h>
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>

#ifdef CONFIG_ARCH_SUN8IW1
static struct cpu_budget_table m_default_budgets_table[]=
{
    {1,1008000,4,INVALID_FREQ,0},
    {1,864000 ,4,INVALID_FREQ,0},
    {1,720000 ,4,INVALID_FREQ,0},
    {1,480000 ,4,INVALID_FREQ,0},
    {1,1008000,3,INVALID_FREQ,0},
    {1,864000 ,3,INVALID_FREQ,0},
    {1,720000 ,3,INVALID_FREQ,0},
    {1,480000 ,3,INVALID_FREQ,0},
    {1,1008000,2,INVALID_FREQ,0},
    {1,864000 ,2,INVALID_FREQ,0},
    {1,720000 ,2,INVALID_FREQ,0},
    {1,480000 ,2,INVALID_FREQ,0},
    {1,1008000,1,INVALID_FREQ,0},
    {1,864000 ,1,INVALID_FREQ,0},
    {1,720000 ,1,INVALID_FREQ,0},
    {1,480000 ,1,INVALID_FREQ,0},
};
#endif
#ifdef CONFIG_ARCH_SUN8IW5
static struct cpu_budget_table m_default_budgets_table[]=
{
    {1,1344000,4,INVALID_FREQ,0},
    {1,1200000 ,4,INVALID_FREQ,0},
    {1,1008000 ,4,INVALID_FREQ,0},
    {1,648000 ,4,INVALID_FREQ,0},
};
#endif
#ifdef CONFIG_ARCH_SUN8IW6
static struct cpu_budget_table m_default_budgets_table[]=
{
    {1,1200000,4,1200000,4},
    {1,1104000,4,1104000,4},
    {1,1008000,4,1008000,4},
    {1,816000,4,816000,4},
    {1,1200000,4,1200000,3},
    {1,1104000,4,1104000,3},
    {1,1008000,4,1008000,3},
    {1,816000,4,816000,3},
    {1,1200000,4,1200000,2},
    {1,1104000,4,1104000,2},
    {1,1008000,4,1008000,2},
    {1,816000,4,816000,2},
    {1,1200000,4,1200000,1},
    {1,1104000,4,1104000,1},
    {1,1008000,4,1008000,1},
    {1,816000,4,816000,1},
};
#endif
#ifdef CONFIG_ARCH_SUN8IW7
static struct cpu_budget_table m_default_budgets_table[]=
{
    {1,1200000 ,4,INVALID_FREQ,0},
    {1,1008000 ,4,INVALID_FREQ,0},
    {1,1008000 ,2,INVALID_FREQ,0},
    {1,1008000 ,1,INVALID_FREQ,0},
    {1,504000  ,1,INVALID_FREQ,0},
};
#endif
#ifdef CONFIG_ARCH_SUN9IW1
static struct cpu_budget_table m_default_budgets_table[]=
{
    {1,1008000,4,1008000,4},
    {1,1008000,4,864000,4},
    {1,1008000,4,720000,4},
    {1,1008000,4,1008000,3},
    {1,1008000,4,864000,3},
    {1,1008000,4,720000,3},
    {1,1008000,4,1008000,2},
    {1,1008000,4,864000,2},
    {1,1008000,4,720000,2},
    {1,1008000,4,1008000,1},
    {1,1008000,4,864000,1},
    {1,1008000,4,720000,1},
    {1,1008000,4,INVALID_FREQ,0},
    {1,864000,4,INVALID_FREQ,0},
    {1,720000,4,INVALID_FREQ,0},
    {1,480000,4,INVALID_FREQ,0},
};

#endif
#define SYSCFG_BUDGET_TABLE_MAX 64
static struct cpu_budget_table m_syscfg_budgets_table[SYSCFG_BUDGET_TABLE_MAX];
static unsigned int m_syscfg_budgets_table_num=0;
static struct cpu_budget_table* dynamic_tbl=NULL;
static struct cpu_budget_table* m_current_tbl=m_default_budgets_table;
static unsigned int dynamic_tbl_num=sizeof(m_default_budgets_table)/sizeof(struct cpu_budget_table);
static unsigned int max_tbl_num=sizeof(m_default_budgets_table)/sizeof(struct cpu_budget_table);
#ifdef CONFIG_SCHED_HMP
extern void __init arch_get_fast_and_slow_cpus(struct cpumask *fast,
									struct cpumask *slow);
#endif
static int sunxi_cpu_budget_cooling_register(struct platform_device *pdev)
{
	struct thermal_cooling_device *cdev;
	struct cpumask cluster0_mask;
	struct cpumask cluster1_mask;
    int i;
	/* make sure cpufreq driver has been initialized */
	if (!cpufreq_frequency_get_table(0))
		return -EPROBE_DEFER;
    cpumask_clear(&cluster0_mask);
    cpumask_clear(&cluster1_mask);
#if defined(CONFIG_SCHED_HMP)
    arch_get_fast_and_slow_cpus(&cluster1_mask,&cluster0_mask);
#elif defined(CONFIG_SCHED_SMP_DCMP)
	if (strlen(CONFIG_CLUSTER0_CPU_MASK) && strlen(CONFIG_CLUSTER1_CPU_MASK)) {
		if (cpulist_parse(CONFIG_CLUSTER0_CPU_MASK, &cluster0_mask)) {
			pr_err("Failed to parse cluster0 cpu mask!\n");
			return -1;
		}
		if (cpulist_parse(CONFIG_CLUSTER1_CPU_MASK, &cluster1_mask)) {
			pr_err("Failed to parse cluster1 cpu mask!\n");
			return -1;
		}
	}
#else
        cpumask_copy(&cluster0_mask, cpu_possible_mask);
#endif
    dynamic_tbl = kmalloc(sizeof(struct cpu_budget_table)*max_tbl_num,GFP_KERNEL);
    dynamic_tbl_num=0;
    for(i=0;i<max_tbl_num;i++)
    {
        if (m_current_tbl[i].online)
        {
            dynamic_tbl[dynamic_tbl_num].cluster0_freq = m_current_tbl[i].cluster0_freq;
            dynamic_tbl[dynamic_tbl_num].cluster0_cpunr = m_current_tbl[i].cluster0_cpunr;
            dynamic_tbl[dynamic_tbl_num].cluster1_freq = m_current_tbl[i].cluster1_freq;
            dynamic_tbl[dynamic_tbl_num].cluster1_cpunr = m_current_tbl[i].cluster1_cpunr;
            dynamic_tbl[dynamic_tbl_num].gpu_throttle = m_current_tbl[i].gpu_throttle;
            dynamic_tbl_num++;
        }
    }

	cdev = cpu_budget_cooling_register(dynamic_tbl,dynamic_tbl_num,&cluster0_mask,&cluster1_mask);
	if (IS_ERR_OR_NULL(cdev)) {
		dev_err(&pdev->dev, "Failed to register cooling device\n");
		return PTR_ERR(cdev);
	}
	platform_set_drvdata(pdev, cdev);
	dev_info(&pdev->dev, "Cooling device registered: %s\n",	cdev->type);
	return 0;
}
static int sunxi_cpu_budget_cooling_unregister(struct platform_device *pdev)
{
    int ret = 0;
	struct thermal_cooling_device *cdev = platform_get_drvdata(pdev);
	cpu_budget_cooling_unregister(cdev);
    kfree(dynamic_tbl);
    return ret;
}

static ssize_t
sunxi_cpu_budget_present_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
    int ret=0;
	struct platform_device *pdev;
	struct thermal_cooling_device *cdev;
    struct cpu_budget_cooling_device *bdevice;

    pdev= to_platform_device(dev);
    if(!pdev)
        return ret;

	cdev = platform_get_drvdata(pdev);
    if((!cdev) || (!cdev->devdata))
        return ret;

	bdevice= (struct cpu_budget_cooling_device *)cdev->devdata;
    ret += sprintf(buf, "Limit:%d,%d,%d,%d,%d\n",
                      bdevice->cluster0_freq_limit,
                      bdevice->cluster0_num_limit,
                      bdevice->cluster1_freq_limit,
                      bdevice->cluster1_num_limit,
                      bdevice->gpu_throttle);

    return ret;
}
static ssize_t
sunxi_cpu_budget_online_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int i,j,ret=0;
    for(i=0,j=0;i<max_tbl_num;i++)
    {
        ret += sprintf(buf, "index:%2d, online:%d, c0freq:%d, c0nums:%d, c1freq:%d, c1nums:%d, gputhrottle:%d\n"
                          ,m_current_tbl[i].online?(j++):-1,m_current_tbl[i].online
                          ,m_current_tbl[i].cluster0_freq
                          ,m_current_tbl[i].cluster0_cpunr
                          ,m_current_tbl[i].cluster1_freq
                          ,m_current_tbl[i].cluster1_cpunr
                          ,m_current_tbl[i].gpu_throttle);
        buf += strlen(buf);
    }
    return ret;
}
static ssize_t
sunxi_cpu_budget_online_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
    unsigned int enable;
	char *endp;
    int i=0;
	struct platform_device *pdev = to_platform_device(dev);
    for(i=0;i<max_tbl_num;i++)
    {
        enable = simple_strtoul(buf, &endp, 10);
        if (*endp != ' ' && *endp != '\n') {
            printk("%s: %d Error\n", __func__, __LINE__);
            return -EINVAL;
        }
        if(!enable)
            m_current_tbl[i].online=0;
        else
            m_current_tbl[i].online=1;
        buf = endp + 1;
    }
    sunxi_cpu_budget_cooling_unregister(pdev);
    sunxi_cpu_budget_cooling_register(pdev);
    return count;
}

extern int cpu_budget_update_state(struct cpu_budget_cooling_device *cpu_budget_device);
static ssize_t
sunxi_cpu_budget_roomage_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
    int ret=0;
	struct platform_device *pdev;
	struct thermal_cooling_device *cdev;
    struct cpu_budget_cooling_device *bdevice;
    pdev= to_platform_device(dev);
    if(!pdev)
        return ret;

	cdev = platform_get_drvdata(pdev);
    if((!cdev) || (!cdev->devdata))
        return ret;
	bdevice= (struct cpu_budget_cooling_device *)cdev->devdata;
    ret += sprintf(buf, "roomage:%d,%d,%d,%d,%d,%d,%d,%d\n",
                      bdevice->cluster0_freq_floor,
                      bdevice->cluster0_num_floor,
                      bdevice->cluster1_freq_floor,
                      bdevice->cluster1_num_floor,
                      bdevice->cluster0_freq_roof,
                      bdevice->cluster0_num_roof,
                      bdevice->cluster1_freq_roof,
                      bdevice->cluster1_num_roof);
    return ret;
}
static ssize_t
sunxi_cpu_budget_roomage_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
    int ret=0;
	struct platform_device *pdev;
	struct thermal_cooling_device *cdev;
    struct cpu_budget_cooling_device *bdevice;
    unsigned int roomage_data[8];
    unsigned long flags;

    pdev= to_platform_device(dev);
    if(!pdev)
        return ret;

	cdev = platform_get_drvdata(pdev);
    if((!cdev) || (!cdev->devdata))
        return ret;
    sscanf(buf,"%u %u %u %u %u %u %u %u\n",
                      &roomage_data[0],
                      &roomage_data[1],
                      &roomage_data[2],
                      &roomage_data[3],
                      &roomage_data[4],
                      &roomage_data[5],
                      &roomage_data[6],
                      &roomage_data[7]);
	bdevice= (struct cpu_budget_cooling_device *)cdev->devdata;
    spin_lock_irqsave(&bdevice->lock, flags);
	bdevice->cluster0_freq_floor = roomage_data[0];
	bdevice->cluster0_num_floor = roomage_data[1];
	bdevice->cluster1_freq_floor = roomage_data[2];
	bdevice->cluster1_num_floor = roomage_data[3];
	bdevice->cluster0_freq_roof = roomage_data[4];
	bdevice->cluster0_num_roof = roomage_data[5];
	bdevice->cluster1_freq_roof = roomage_data[6];
	bdevice->cluster1_num_roof = roomage_data[7];
    spin_unlock_irqrestore(&bdevice->lock, flags);
    ret = cpu_budget_update_state(bdevice);
    if(ret)
        return ret;
    return count;
}

static DEVICE_ATTR(roomage, 0644,sunxi_cpu_budget_roomage_show, sunxi_cpu_budget_roomage_store);
static DEVICE_ATTR(online, 0644,sunxi_cpu_budget_online_show, sunxi_cpu_budget_online_store);
static DEVICE_ATTR(present, 0444,sunxi_cpu_budget_present_show, NULL);

static int sunxi_cpu_budget_cooling_probe(struct platform_device *pdev)
{
    int ret=0;
    ret=sunxi_cpu_budget_cooling_register(pdev);
    device_create_file(&pdev->dev, &dev_attr_online);
    device_create_file(&pdev->dev, &dev_attr_present);
    device_create_file(&pdev->dev, &dev_attr_roomage);
    return ret;
}
static int sunxi_cpu_budget_cooling_remove(struct platform_device *pdev)
{
    int ret=0;
    device_remove_file(&pdev->dev, &dev_attr_roomage);
    device_remove_file(&pdev->dev, &dev_attr_present);
    device_remove_file(&pdev->dev, &dev_attr_online);
    ret=sunxi_cpu_budget_cooling_unregister(pdev);
    return ret;
}

static struct platform_device sunxi_cpu_budget_cooling_device = {
	.name		    = "sunxi-budget-cooling",
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver sunxi_cpu_budget_cooling_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sunxi-budget-cooling",
	},
	.probe = sunxi_cpu_budget_cooling_probe,
	.remove = sunxi_cpu_budget_cooling_remove,
};

#ifdef CONFIG_SUNXI_BUDGET_COOLING_VFTBL
struct cpufreq_dvfs_s {
    unsigned int freq;   /* cpu frequency */
    unsigned int volt;   /* voltage for the frequency */
};
static struct cpufreq_dvfs_s c0_vftable_syscfg[16];
static struct cpufreq_dvfs_s c1_vftable_syscfg[16];
static unsigned int c0_table_length=0;
static unsigned int c1_table_length=0;

#if defined(CONFIG_ARCH_SUN9IW1)|| defined(CONFIG_ARCH_SUN8IW6)
static char* multi_cluster_prefix[]={"L_LV","B_LV"};
#else
static char* single_cluster_prefix[]={"LV"};
#endif

static int sunxi_cpu_budget_vftable_init(void)
{
    script_item_u val;
    script_item_value_type_e type;
    struct cpufreq_dvfs_s *ptable;
    unsigned int * plen;
    unsigned int freq,volt;
    int i,j,k,cluster_nr=1,ret = -1;
    char tbl_name[]="dvfs_table";
    char name[16] = {0};
    unsigned int vf_table_type = 0;
    char** prefix;

#if defined(CONFIG_ARCH_SUN9IW1)|| defined(CONFIG_ARCH_SUN8IW6)
    cluster_nr = 2;
    prefix = multi_cluster_prefix;
#else
    cluster_nr = 1;
    prefix = single_cluster_prefix;
#endif

    type = script_get_item("dvfs_table", "vf_table_count", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_info("%s: support only one vf_table\n", __func__);
        sprintf(tbl_name, "%s", "dvfs_table");
    } else {
        vf_table_type = sunxi_get_soc_bin();
        sprintf(tbl_name, "%s%d", "vf_table", vf_table_type);
    }

    for(k=0;k<cluster_nr;k++)
    {
        ptable = k?&c1_vftable_syscfg[0]:&c0_vftable_syscfg[0];
        plen =  k?&c1_table_length:&c0_table_length;
        sprintf(name, "%s_count",prefix[k]);
        type = script_get_item(tbl_name, name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
            pr_err("fetch %s from sysconfig failed\n",name);
            goto fail;
        }
        *plen = val.val;

        if(*plen >= 16){
            pr_err("%s from sysconfig is out of bounder\n",name);
            goto fail;
        }
        for (i = 1,j=0; i <= *plen ; i++)
        {
            sprintf(name, "%s%d_freq", prefix[k],i);
            type = script_get_item(tbl_name, name, &val);
            if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                pr_err("get %s from sysconfig failed\n", name);
                goto fail;
            }
            freq = val.val;

            sprintf(name, "%s%d_volt", prefix[k],i);
            type = script_get_item(tbl_name, name, &val);
            if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
                pr_err("get %s from sysconfig failed\n", name);
                goto fail;
            }
            volt = val.val;

            if(freq)
            {
                ptable[j].freq = freq / 1000;
                ptable[j].volt = volt;
                j++;
             }
        }
        *plen = j;
    }
    return 0;
fail:
    return ret;
}
#endif
static int sunxi_cpu_budget_syscfg_init(void)
{
    script_item_u val;
    script_item_value_type_e type;
    char tbl_name[]="cooler_table";
    int i,num=-1,ret = -1;
    char name[32] = {0};

    type = script_get_item(tbl_name, "cooler_count", &val);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("fetch cooler_count from sysconfig failed\n");
        goto fail;
    }

    m_syscfg_budgets_table_num = val.val;
    if(m_syscfg_budgets_table_num >= SYSCFG_BUDGET_TABLE_MAX){
        printk("cooler_count from sysconfig is out of bounder\n");
        goto fail;
    }
    memset(m_syscfg_budgets_table,0x0,sizeof(m_syscfg_budgets_table));
    for (i = 0; i < m_syscfg_budgets_table_num; i++){
        sprintf(name, "cooler%d", i);
        type = script_get_item(tbl_name, name, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
            printk("get cooler%d from sysconfig failed\n", i);
            goto fail;
        }
#if defined(CONFIG_ARCH_SUN9IW1)
            num=sscanf(val.str, "%u %u %u %u %u",
                                        &m_syscfg_budgets_table[i].cluster0_freq,
                                        &m_syscfg_budgets_table[i].cluster0_cpunr,
                                        &m_syscfg_budgets_table[i].cluster1_freq,
                                        &m_syscfg_budgets_table[i].cluster1_cpunr,
                                        &m_syscfg_budgets_table[i].gpu_throttle);
#elif defined(CONFIG_ARCH_SUN8IW5) || defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW7)
        num=sscanf(val.str, "%u %u %u %u %u",
                                        &m_syscfg_budgets_table[i].cluster0_freq,
                                        &m_syscfg_budgets_table[i].cluster0_cpunr,
                                        &m_syscfg_budgets_table[i].cluster1_freq,
                                        &m_syscfg_budgets_table[i].cluster1_cpunr,
                                        &m_syscfg_budgets_table[i].gpu_throttle);
#endif
        if (num <= -1) {
            printk("parse cooler%d from sysconfig failed\n", i);
            goto fail;
        }
        m_syscfg_budgets_table[i].online = 1;
    }
    dynamic_tbl_num = m_syscfg_budgets_table_num;
    max_tbl_num = m_syscfg_budgets_table_num;
    m_current_tbl = m_syscfg_budgets_table;
#ifdef CONFIG_SUNXI_BUDGET_COOLING_VFTBL
    if((!m_syscfg_budgets_table[0].cluster0_freq)  && (!m_syscfg_budgets_table[0].cluster1_freq))
        for (i = 0; i < m_syscfg_budgets_table_num; i++)
        {
            if(m_syscfg_budgets_table[i].cluster0_freq < c0_table_length)
                m_syscfg_budgets_table[i].cluster0_freq = c0_vftable_syscfg[m_syscfg_budgets_table[i].cluster0_freq].freq;
            else
                m_syscfg_budgets_table[i].cluster0_freq = c0_vftable_syscfg[c0_table_length-1].freq;
            if(m_syscfg_budgets_table[i].cluster1_freq < c1_table_length)
                m_syscfg_budgets_table[i].cluster1_freq = c1_vftable_syscfg[m_syscfg_budgets_table[i].cluster1_freq].freq;
            else
                m_syscfg_budgets_table[i].cluster1_freq = c1_vftable_syscfg[c1_table_length-1].freq;
        }
#endif
    return 0;

fail:
    m_syscfg_budgets_table_num=0;
    return ret;
}

static int __init sunxi_cpu_budget_cooling_init(void)
{
	int ret = 0;
#ifdef CONFIG_SUNXI_BUDGET_COOLING_VFTBL
    sunxi_cpu_budget_vftable_init();
#endif
    sunxi_cpu_budget_syscfg_init();
	ret = platform_driver_register(&sunxi_cpu_budget_cooling_driver);
	if (IS_ERR_VALUE(ret)) {
		printk("register sunxi_cpu_budget_cooling_driver failed\n");
		return ret;
	}
	ret = platform_device_register(&sunxi_cpu_budget_cooling_device);
	if (IS_ERR_VALUE(ret)) {
		printk("register sunxi_cpu_budget_cooling_device failed\n");
		return ret;
	}
	return ret;
}

static void __exit sunxi_cpu_budget_cooling_exit(void)
{
	platform_driver_unregister(&sunxi_cpu_budget_cooling_driver);
}

/* Should be later than sunxi_cpufreq register */
late_initcall(sunxi_cpu_budget_cooling_init);
module_exit(sunxi_cpu_budget_cooling_exit);

MODULE_AUTHOR("kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi cpufreq cooling driver");
MODULE_LICENSE("GPL");

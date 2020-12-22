/*
 * linux/drivers/sunxi_module/sunxi_module.c
 *
 * Copyright(c) 2014-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: superm<superm@allwinnertech.com>
 *
 * allwinner sunxi modules manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/syscore_ops.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sunxi_module.h>

#include <mach/platform.h>

#define DEV_NAME	"sunxi-module"
#define DRV_NAME	"sunxi-module"
#define DRV_VERSION     "v0.1"

/*
 * @module_info : the struct of module info;
 * @list: the list of module info;
 */
typedef struct sunxi_module_node
{
	struct sunxi_module_info module_info;
	struct list_head list;
}sunxi_module_node_t;

struct sunxi_module_node module_head;

int sunxi_module_info_register(struct sunxi_module_info *module_info)
{
	struct sunxi_module_node *module_node;

	module_node = (struct sunxi_module_node *)kmalloc(sizeof(struct sunxi_module_node), GFP_KERNEL);
	memcpy((void *)&module_node->module_info, (const void *)module_info, sizeof(struct sunxi_module_info));
	list_add_tail(&module_node->list, &module_head.list);
	printk("[sunxi-module]: %s register success\n", module_node->module_info.module);

	return 0;
}
EXPORT_SYMBOL(sunxi_module_info_register);

int sunxi_module_info_unregister(struct sunxi_module_info *module_info)
{
	struct list_head *pos, *n;
	struct sunxi_module_node *p;

	list_for_each_safe(pos, n, &module_head.list) {
		p = list_entry(pos, struct sunxi_module_node, list);
		if (!strcmp((const char *)p->module_info.module, (const char *)module_info->module)) {
			list_del(pos);
			kfree(p);
			printk("[sunxi-module]: %s unregister success\n", module_info->module);

			return 0;
		}
	}

	printk("[sunxi-module]: %s unregister fail\n", module_info->module);

	return 1;
}
EXPORT_SYMBOL(sunxi_module_info_unregister);

static int module_proc_show(struct seq_file *seq, void *v)
{
	struct list_head *pos;
	struct sunxi_module_node *p;

	seq_printf(seq, "sunxi modules info:\n");
	seq_printf(seq, "-------------------------------------------\n");
	seq_printf(seq, "|module              |version             |\n");
	seq_printf(seq, "-------------------------------------------\n");

	list_for_each(pos, &module_head.list){
	p=list_entry(pos, struct sunxi_module_node, list);
	seq_printf(seq, "|%-16s    |%-16s    |\n", p->module_info.module, p->module_info.version);
	seq_printf(seq, "-------------------------------------------\n");
    	}

	return 0;
}

int sunxi_module_probe(struct platform_device *pdev)
{
	INIT_LIST_HEAD(&module_head.list);
	printk("[sunxi-module]: [%s] probe success\n", dev_name(&pdev->dev));

	return 0;
}

int sunxi_module_remove(struct platform_device *pdev)
{
	struct list_head *pos, *n;
	struct sunxi_module_node *p;

	list_for_each_safe(pos, n, &module_head.list) {
		list_del(pos);
		p = list_entry(pos, struct sunxi_module_node, list);
		kfree(p);
	}

	printk("[sunxi-module]: [%s] remove success\n", dev_name(&pdev->dev));

	return 0;
}

static struct platform_device module_device = {
	.name = DEV_NAME,
};
static struct platform_driver module_driver = {
	.probe = sunxi_module_probe,
	.remove = sunxi_module_remove,
	.driver = {.name = DRV_NAME}
};

#ifdef CONFIG_PROC_FS
static int module_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, module_proc_show, NULL);
}

static const struct file_operations module_proc_fops = {
	.owner = THIS_MODULE,
	.open = module_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int __init sunxi_module_init(void)
{
	int ret;

#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *parent;
	parent = proc_mkdir("sunxi", NULL);
	proc_create("module-version", S_IRUGO, parent, &module_proc_fops);
#endif

	ret = platform_device_register(&module_device);
	if(ret)
		return ret;
	return platform_driver_register(&module_driver);
}

static void __exit sunxi_module_exit(void)
{
#ifdef CONFIG_PROC_FS
	remove_proc_entry("sunxi/module-version", NULL);
#endif
	platform_driver_unregister(&module_driver);
	platform_device_unregister(&module_device);
}

arch_initcall(sunxi_module_init);
module_exit(sunxi_module_exit);
MODULE_DESCRIPTION("sunxi module driver");
MODULE_AUTHOR("superm<superm@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:sunxi module driver");


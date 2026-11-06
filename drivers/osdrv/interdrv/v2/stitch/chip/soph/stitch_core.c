#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/compat.h>

#include <base_ctx.h>
#include "comm_stitch.h"

#include <base_cb.h>
#include <base_ctx.h>
#include "stitch.h"
#include "stitch_cb.h"
#include "stitch_debug.h"
#include "stitch_core.h"
#include "stitch_ioctl.h"
#include "stitch_proc.h"
#include "stitch_reg_cfg.h"


#define STITCH_DEV_NAME "soph-stitch"
#define CONFIG_PM_SLEEP 1

static atomic_t stitch_open_count = ATOMIC_INIT(0);
static atomic_t suspend_flag = ATOMIC_INIT(0);
static struct stitch_dev *g_stitch_dev;

int gStitchDumpReg;
module_param(gStitchDumpReg, int, 0644);
MODULE_PARM_DESC(gStitchDumpReg, "Stitch Dump reg");

bool gStitchDumpDmaCfg;
module_param(gStitchDumpDmaCfg, bool, 0644);
MODULE_PARM_DESC(gStitchDumpDmaCfg, "Stitch Dump DMA cfg");

unsigned int stitch_log_lv = DBG_WARN;
module_param(stitch_log_lv, int, 0644);
MODULE_PARM_DESC(stitch_log_lv, "Stitch Debug Log Level");

unsigned int clk_sys_freq = 0;
module_param(clk_sys_freq, int, 0644);
MODULE_PARM_DESC(clk_sys_freq, "clk_sys_freq setting by user");

atomic_t * stitch_get_open_count(void)
{
	return &stitch_open_count;
}

struct stitch_dev * stitch_get_dev(void)
{
	return g_stitch_dev;
}

void stitch_enable_dev_clk(bool en)
{
	struct stitch_dev *dev = stitch_get_dev();

	if (!dev || !dev->clk_stitch) {
		TRACE_STITCH(DBG_ERR, "null dev or null clk_stitch\n");
		return;
	}

	if (en) {
		if (!__clk_is_enabled(dev->clk_stitch))
			clk_enable(dev->clk_stitch);
		//TRACE_STITCH(DBG_WARN, "stitch_enable_dev_clk\n");
	} else {
		if (__clk_is_enabled(dev->clk_stitch))
			clk_disable(dev->clk_stitch);
		//sTRACE_STITCH(DBG_WARN, "stitch_disable_dev_clk\n");
	}
}

int stitch_core_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	struct stitch_dev *vdev = (struct stitch_dev *)dev;
	int rc = -1;
	(void)caller;
	(void)vdev;

	switch (cmd) {
	case STITCH_CB_QBUF_TRIGGER:
	{
		break;
	}
	default:
		break;
	}

	return rc;
}

static int stitch_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_STITCH);
}

static int stitch_core_register_cb(struct stitch_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_STITCH;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= stitch_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static void stitch_job_finish(struct stitch_job *job, struct stitch_dev *dev)
{
	if (atomic_read(&dev->state) != STITCH_DEV_STATE_END)
		return;

	atomic_set(&dev->state, STITCH_DEV_STATE_IDLE);

	if (atomic_read(&job->job_state) == STITCH_JOB_WORKING) {
		if (job->fn_job_cb)
			job->fn_job_cb(job->data);
	}
}

static void stitch_irq_handler(unsigned char intr_status, struct stitch_dev *dev)
{
	struct stitch_job *job = dev->job;
	(void)(intr_status);

	if (!job) {
		TRACE_STITCH(DBG_ERR, "job is NULL\n");
		return;
	}

	atomic_set(&dev->state, STITCH_DEV_STATE_END);

	stitch_job_finish(job, dev);
}

static irqreturn_t stitch_isr(int irq, void *_dev)
{
	unsigned char intr_status;
	struct stitch_dev *dev = (struct stitch_dev *)_dev;
	struct __stitch_ctx **stitch_ctx = stitch_get_ctx();
	struct stitch_handler_ctx *evt_hdl_ctx = stitch_get_evt_hdl_ctx();
	stitch_grp grp_id = evt_hdl_ctx->working_grp;

	if (dev && (dev->irq_num == irq)) {
		intr_status = stitch_intr_status();
		TRACE_STITCH(DBG_DEBUG, "irq(%d) for grp(%d), status(0x%x)\n", irq, grp_id, intr_status);

		stitch_intr_clr();
		stitch_disable();
		stitch_invalid_param(1);
		stitch_invalid_param(0);
		//stitch_reset_init(); //if neessary, but intr must clr,otherwise cpu hang
		//stitch_enable_dev_clk(false);

		if (stitch_ctx[grp_id]) {
			dev->job = &stitch_ctx[grp_id]->job;
			stitch_irq_handler(intr_status, dev);
		}
	}

	return IRQ_HANDLED;
}

static void stitch_dev_init(struct stitch_dev *dev)
{
	if (!dev) {
		TRACE_STITCH(DBG_ERR, "null dev.\n");
		return;
	}

	spin_lock_init(&dev->lock);
	atomic_set(&dev->state, STITCH_DEV_STATE_RUNNING);
	dev->job = NULL;

	if (dev->clk_src)
		clk_prepare_enable(dev->clk_src);
	if (dev->clk_apb)
		clk_prepare_enable(dev->clk_apb);
	if (dev->clk_stitch && !__clk_is_enabled(dev->clk_stitch))
		clk_prepare_enable(dev->clk_stitch);

	stitch_enable_dev_clk(false);

	if (clk_sys_freq)
		dev->clk_sys_freq = clk_sys_freq;
}

static void stitch_dev_deinit(struct stitch_dev *dev)
{
	if (!dev) {
		TRACE_STITCH(DBG_ERR, "null dev.\n");
		return;
	}

	if (dev->clk_stitch && __clk_is_enabled(dev->clk_stitch))
		clk_disable_unprepare(dev->clk_stitch);
	if (dev->clk_apb)
		clk_disable_unprepare(dev->clk_apb);
	if (dev->clk_src)
		clk_disable_unprepare(dev->clk_src);

	if (clk_sys_freq) {
		dev->clk_sys_freq = 0;
		clk_sys_freq = 0;
	}
	atomic_set(&dev->state, STITCH_DEV_STATE_IDLE);
}

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	const char stitch_name[16] = "stitch";
	const char stitch_clk_name[16] = "clk_stitch";
	const char stitch_clk_sys_name[16] = {"clk_sys_3"};
	int irq_num = -1;
	struct resource *res = NULL;
	struct stitch_dev *dev = NULL;
	void *reg_base = NULL;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get stitch drvdata\n");
		return -EINVAL;
	}

	//init res
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!res)) {
		TRACE_STITCH(DBG_ERR, "Failed to get stitch res\n");
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	reg_base = devm_ioremap(&pdev->dev, res->start,
					    res->end - res->start);
	//reg_base = devm_ioremap_resource(&pdev->dev, res[i]);
#else
	reg_base = devm_ioremap_nocache(&pdev->dev, res->start,
					    res->end - res->start);
#endif

	TRACE_STITCH(DBG_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n"
		, res->start, res->end, reg_base);
	if (!reg_base) {
		TRACE_STITCH(DBG_ERR, "Failed to get reg_base for stitch\n");
		return -EINVAL;
	}

	stitch_set_base_addr(reg_base + 0x1e00, reg_base + 0x100);

	/* Interrupt res*/
	irq_num = platform_get_irq_byname(pdev, stitch_name);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for irq_num[%d] %s\n", irq_num, stitch_name);
		return -ENODEV;
	}
	TRACE_STITCH(DBG_INFO, "irq(%d) for %s get from platform driver.\n", irq_num, stitch_name);
	dev->irq_num = irq_num;

	//clk res
	dev->clk_src = devm_clk_get(&pdev->dev, stitch_clk_sys_name);
	if (IS_ERR(dev->clk_src)) {
		TRACE_STITCH(DBG_ERR, "Cannot get clk for clk_sys_3 for stitch\n");
		dev->clk_src = NULL;
	}
	dev->clk_stitch = devm_clk_get(&pdev->dev, stitch_clk_name);
	if (IS_ERR(dev->clk_stitch)) {
		TRACE_STITCH(DBG_ERR, "Cannot get clk for clk_stitch\n");
		dev->clk_stitch = NULL;
	}
#else
	TRACE_STITCH(DBG_INFO, "cannot get res for stitch\n");
	rc = -1;
#endif
	return rc;
}

static int stitch_open(struct inode *inode, struct file *filep)
{
	struct stitch_dev *dev
		= container_of(filep->private_data, struct stitch_dev, miscdev);
	int i;

	pr_info("stitch_open\n");

	if (!dev) {
		pr_err("Cannot find stitch private data\n");
		return -ENODEV;
	}

	i = atomic_inc_return(&stitch_open_count);
	if (i > 1) {
		pr_info("stitch_open: open %d times\n", i);
		return 0;
	}

	stitch_reset();
	stitch_dma_cfg_clr_all();

	return 0;
}

static int stitch_release(struct inode *inode, struct file *filep)
{
	struct stitch_dev *dev
		= container_of(filep->private_data, struct stitch_dev, miscdev);
	int i;

	pr_info("stitch_release\n");

	if (!dev) {
		pr_err("Cannot find stitch private data\n");
		return -ENODEV;
	}

	i = atomic_dec_return(&stitch_open_count);
	if (i) {
		pr_info("stitch_close: open %d times\n", i);
		return 0;
	}

	stitch_reset();
	stitch_dma_cfg_clr_all();

	return 0;
}

#ifdef CONFIG_COMPAT
static long stitch_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations stitch_fops = {
	.owner = THIS_MODULE,
	.open = stitch_open,
	.release = stitch_release,
	.unlocked_ioctl = stitch_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = stitch_compat_ptr_ioctl,
#endif
};

static int register_stitch_dev(struct device *dev, struct stitch_dev *bdev)
{
	int rc;

	bdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	bdev->miscdev.name = devm_kasprintf(dev, GFP_KERNEL, STITCH_DEV_NAME);
	bdev->miscdev.fops = &stitch_fops;

	rc = misc_register(&bdev->miscdev);
	if (rc)
		pr_err("stitch_dev: failed to register misc device.\n");

	return rc;
}

static const struct of_device_id stitch_dt_match[] = {
	{.compatible = "cvitek,stitch"},
	{}
};

static int stitch_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct stitch_dev *dev;
	const struct of_device_id * match;

	match = of_match_device(stitch_dt_match, &pdev->dev);
	if (!match)
		return ENODEV;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		TRACE_STITCH(DBG_ERR, "Failed to allocate resource\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, dev);

	g_stitch_dev = dev;
	rc = _init_resources(pdev);
	if (rc) {
		TRACE_STITCH(DBG_ERR, "Failed to init resource\n");
		goto err_dev;
	}

	rc = register_stitch_dev(&pdev->dev, dev);
	if (rc) {
		TRACE_STITCH(DBG_ERR, "Failed to register dev\n");
		goto err_dev;
	}

	rc = stitch_proc_init(dev);
	if (rc) {
		TRACE_STITCH(DBG_ERR, "Failed to init proc\n");
		goto err_proc;
	}

	if (devm_request_irq(&pdev->dev, dev->irq_num, stitch_isr
		, IRQF_SHARED, "stitch_irq", (void *)dev)) {
		dev_err(&pdev->dev, "Unable to request stitch IRQ(%d)\n", dev->irq_num);
		goto err_irq;
	}

	if (stitch_core_register_cb(dev)) {
		dev_err(&pdev->dev, "Failed to register vpss cb, err %d\n", rc);
		goto err_irq;
	}

	rc = stitch_thread_init();
	if (rc)
		goto err_kthread;

	stitch_dev_init(dev);

	TRACE_STITCH(DBG_INFO, "stitch init done\n");
	return rc;

err_kthread:
err_irq:
	stitch_proc_remove(dev);
err_proc:
	misc_deregister(&dev->miscdev);
err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	TRACE_STITCH(DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

static int stitch_remove(struct platform_device *pdev)
{
	struct stitch_dev *dev;

	stitch_thread_deinit();

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get stitch drvdata");
		return -EINVAL;
	}

	stitch_dev_deinit(dev);

	if (stitch_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm stitch cb\n");
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get stitch drvdata");
		return -EINVAL;
	}

	stitch_proc_remove(dev);
	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);
	g_stitch_dev = NULL;

	return 0;
}

#ifdef CONFIG_PM_SLEEP
int stitch_suspend(struct device *dev)
{
	int ret = 0;
	struct stitch_dev *stit_dev = stitch_get_dev();

	/*step 1 suspend envent_handlers*/
	ret = stitch_suspend_handler();
	if (ret) {
		TRACE_STITCH(DBG_ERR, "fail to suspend stitch thread, err=%d\n", ret);
		return -1;
	}

	/*step 2 turn off clock*/
	stitch_dev_deinit(stit_dev);
	atomic_set(&suspend_flag, 1);

	TRACE_STITCH(DBG_WARN, "stitch suspend+\n");
	return 0;
}

int stitch_resume(struct device *dev)
{
	int ret = 0;
	struct stitch_dev *stit_dev = stitch_get_dev();

	/*step 1 turn on clock*/
	stitch_dev_init(stit_dev);

	/*step 3 resume envent_handlers*/
	ret = stitch_resume_handler();
	if (ret) {
		TRACE_STITCH(DBG_ERR, "fail to resume stitch thread, err=%d\n", ret);
		return -1;
	}
	atomic_set(&suspend_flag, 0);

	TRACE_STITCH(DBG_WARN, "stitch resume+\n");
	return 0;
}

bool is_stitch_suspended(void)
{
	return atomic_read(&suspend_flag) == 1 ? true : false;
}

static SIMPLE_DEV_PM_OPS(stitch_pm_ops, stitch_suspend, stitch_resume);
#else
static SIMPLE_DEV_PM_OPS(stitch_pm_ops, NULL, NULL);
#endif

#if (!DEVICE_FROM_DTS)
static void stitch_pdev_release(struct device *dev)
{
}

static struct platform_device stitch_pdev = {
	.name		= "stitch",
	.dev.release	= stitch_pdev_release,
};
#endif

static struct platform_driver stitch_pdrv = {
	.probe      = stitch_probe,
	.remove     = stitch_remove,
	.driver     = {
		.name		= "stitch",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= of_match_ptr(stitch_dt_match),
#endif
		.pm		= &stitch_pm_ops,
	},
};

#if 1
static int __init __stitch_init(void)
{
	int rc;

	TRACE_STITCH(DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&stitch_pdrv);
	#else
	rc = platform_device_register(&stitch_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&stitch_pdrv);
	if (rc)
		platform_device_unregister(&stitch_pdev);
	#endif

	return rc;
}

static void __exit __stitch_exit(void)
{
	TRACE_STITCH(DBG_INFO, " +\n");
	platform_driver_unregister(&stitch_pdrv);
	#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&stitch_pdev);
	#endif
}
module_init(__stitch_init);
module_exit(__stitch_exit);
#else
module_platform_driver(stitch_pdrv);
#endif

MODULE_DESCRIPTION("Cvitek Video Driver For Stitch");
MODULE_AUTHOR("robin.lee");
MODULE_LICENSE("GPL");

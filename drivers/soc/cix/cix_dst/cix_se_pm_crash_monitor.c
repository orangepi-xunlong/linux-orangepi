#include <linux/err.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <mntn_subtype_exception.h>

#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>

#define MBOX_MAX_MSG_LEN	    (16)
#define CIX_MEM_REG_NUM         (2)
#define RDR_DUMP_COMP_TIMEOUT   (500)

#define SRC_SE_CRASH            (0x40000001UL)
#define SRC_PM_CRASH            (0x40000002UL)

struct cix_sp_crash_monitor {
	struct device		    *dev;
	struct mbox_chan	    *rx_channel;
	char			        *rx_buffer;
	char			        *message;
	spinlock_t		        lock;
	wait_queue_head_t	    waitq;
	struct work_struct      pm_work;
	struct work_struct      se_work;
	struct workqueue_struct *workqueue;
};

/**
 * struct cix_mem_region - memory region structure
 * @name: Name of the memory region
 * @da: Device address of the memory region from SE view
 * @sa: System Bus address used to access the memory region
 * @len: Length of the memory region
 */
struct cix_mem_region {
	const char *name;
	u32        da;
	u64        sa;
	u32        len;
};

struct rdr_dump_mem_region {
	const char *name;
	void *va;
	u32 length;
};
static struct rdr_dump_mem_region g_se_rdr_dump_mem_region, g_pm_rdr_dump_mem_region;

enum RDR_PM_MODID {
	// for CSU SE
	RDR_CSUSE_MODID_START = PLAT_BB_MOD_CSUSE_START,
	RDR_CSUSE_SOC_ISR_ERR_MODID,
	RDR_CSUSE_MODID_END = PLAT_BB_MOD_CSUSE_END,
	// for CSU PM
	RDR_CSUPM_MODID_START = PLAT_BB_MOD_CSUPM_START,
	RDR_CSUPM_SOC_ISR_ERR_MODID,
	RDR_CSUPM_MODID_END = PLAT_BB_MOD_CSUPM_END,
};

static struct cix_mem_region sky1_se_mem[1] = {
	{ .name = "se_ddr", .da = RDR_SE_EXCEPTION_AREA, .sa = RDR_SE_EXCEPTION_AREA, .len = RDR_SE_EXCEPTION_AREA_LEN },  /* 512 B */
};

static struct cix_mem_region sky1_pm_mem[1] = {
	{ .name = "pm_ddr", .da = RDR_PM_EXCEPTION_AREA, .sa = RDR_PM_EXCEPTION_AREA, .len = RDR_PM_EXCEPTION_AREA_LEN },  /* 512 B */
};

//static struct completion g_se_rdr_dump_comp, g_pm_rdr_dump_comp;
static struct rdr_register_module_result g_se_current_info, g_pm_current_info;

static struct rdr_exception_info_s g_se_einfo[] = {
	{ { 0, 0 }, RDR_CSUSE_SOC_ISR_ERR_MODID, RDR_CSUSE_SOC_ISR_ERR_MODID, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_CSUSE, RDR_CSUSE, RDR_CSUSE,
	 (u32)RDR_REENTRANT_DISALLOW, CSUSE_S_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "csuse", "csuse isr proc", 0, 0, 0 },
};

static struct rdr_exception_info_s g_pm_einfo[] = {
	{ { 0, 0 }, RDR_CSUPM_SOC_ISR_ERR_MODID, RDR_CSUPM_SOC_ISR_ERR_MODID, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_CSUPM, RDR_CSUPM, RDR_CSUPM,
	 (u32)RDR_REENTRANT_DISALLOW, CSUPM_S_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "csupm", "csupm isr proc", 0, 0, 0 },
};

/*
 * Description : register exceptionwith to rdr
 */
static void rdr_sky1_se_register_exception(void)
{
	unsigned int i;
	u32 ret;

	for (i = 0; i < sizeof(g_se_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u \n", g_se_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_se_einfo[i]);
		if (ret == 0) {
			pr_err("rdr_register_exception %d fail, ret = [%u]\n", i,ret);
			return;
		}
	}
}

static void rdr_sky1_pm_register_exception(void)
{
	unsigned int i;
	u32 ret;

	for (i = 0; i < sizeof(g_pm_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u \n", g_pm_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_pm_einfo[i]);
		if (ret == 0) {
			pr_err("rdr_register_exception %d fail, ret = [%u]\n",i, ret);
			return;
		}
	}
}

static void rdr_sky1_sp_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(g_se_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_se_einfo[i].e_exce_type);
		rdr_unregister_exception(g_se_einfo[i].e_modid);
	}

	for (i = 0; i < sizeof(g_pm_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_pm_einfo[i].e_exce_type);
		rdr_unregister_exception(g_pm_einfo[i].e_modid);
	}
}

static void rdr_sky1_se_dump(u32 modid, u32 etype,
				u64 coreid, char *log_path,
				pfn_cb_dump_done pfn_cb)
{
	int ret;

	ret = rdr_savebuf2fs(log_path, g_se_rdr_dump_mem_region.name,
						g_se_rdr_dump_mem_region.va, g_se_rdr_dump_mem_region.length, 0);
	if (ret < 0) {
		pr_err("rdr_savebuf2fs region_0: name = %s, error = %d\n",
				g_se_rdr_dump_mem_region.name, ret);
		return;
	}

	pr_debug("dump to file successed: path = %s, name = %s, len = 0x%x\n",
			log_path, g_se_rdr_dump_mem_region.name, g_se_rdr_dump_mem_region.length);

	if (pfn_cb)
		pfn_cb(modid, coreid);
}


static void rdr_sky1_pm_dump(u32 modid, u32 etype,
				u64 coreid, char *log_path,
				pfn_cb_dump_done pfn_cb)
{
	int ret;

	ret = rdr_savebuf2fs(log_path, g_pm_rdr_dump_mem_region.name,
						g_pm_rdr_dump_mem_region.va, g_pm_rdr_dump_mem_region.length, 0);
	if (ret < 0) {
		pr_err("rdr_savebuf2fs region_0: name = %s, error = %d\n",
				g_pm_rdr_dump_mem_region.name, ret);
		return;
	}

	pr_debug("dump to file successed: path = %s, name = %s, len = 0x%x\n",
			log_path, g_pm_rdr_dump_mem_region.name, g_pm_rdr_dump_mem_region.length);

	if (pfn_cb)
		pfn_cb(modid, coreid);
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int rdr_sky1_sp_register_core(void)
{
	struct rdr_module_ops_pub s_se_ops, s_pm_ops;
	struct rdr_register_module_result retinfo;
	int ret;
	u64 coreid;

	coreid = RDR_CSUSE;

	s_se_ops.ops_dump = rdr_sky1_se_dump;
	s_se_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_se_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_se_current_info.log_addr = retinfo.log_addr;
	g_se_current_info.log_len = retinfo.log_len;
	g_se_current_info.nve = retinfo.nve;
	pr_debug("%s,%d: SE addr=0x%llx, len=0x%x\n",
		__func__, __LINE__, g_se_current_info.log_addr, g_se_current_info.log_len);

    /***********************************************************************/

	memset(&retinfo, 0, sizeof(struct rdr_register_module_result));

	coreid = RDR_CSUPM;

	s_pm_ops.ops_dump = rdr_sky1_pm_dump;
	s_pm_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_pm_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_pm_current_info.log_addr = retinfo.log_addr;
	g_pm_current_info.log_len = retinfo.log_len;
	g_pm_current_info.nve = retinfo.nve;
	pr_debug("%s,%d: PM addr=0x%llx, len=0x%x\n",
		__func__, __LINE__, g_pm_current_info.log_addr, g_pm_current_info.log_len);


	return ret;
}

static void rdr_sky1_sp_unregister_core(void)
{
	rdr_unregister_module_ops(RDR_CSUSE);
	rdr_unregister_module_ops(RDR_CSUPM);
}

static void sp_mbox_receive_message(struct mbox_client *client, void *message)
{
	struct cix_sp_crash_monitor *mdev = dev_get_drvdata(client->dev);
	unsigned long flags;
	unsigned int *data = (unsigned int *)message;

	spin_lock_irqsave(&mdev->lock, flags);
	if (message) {
		memcpy(mdev->rx_buffer, message, MBOX_MAX_MSG_LEN);
	}

	if (SRC_SE_CRASH == data[1]) {

		queue_work(mdev->workqueue, &mdev->se_work);

	} else if (SRC_PM_CRASH == data[1]) {

		queue_work(mdev->workqueue, &mdev->pm_work);

	} else {
		pr_err("%s, %d, illegal crash source !\n", __func__, __LINE__);
	}

	spin_unlock_irqrestore(&mdev->lock, flags);
	wake_up_interruptible(&mdev->waitq);

}

static struct mbox_chan* cix_sp_mbox_request_channel(struct platform_device *pdev, const char *name)
{
	struct mbox_client *client;
	struct mbox_chan *channel;

	client = devm_kzalloc(&pdev->dev, sizeof(*client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	client->dev		    = &pdev->dev;
	client->rx_callback	= sp_mbox_receive_message;

	channel = mbox_request_channel_byname(client, name);
	if (IS_ERR(channel)) {
		dev_warn(&pdev->dev, "Failed to request %s channel\n", name);
		return NULL;
	}

	return channel;
}

static int cix_sp_rproc_addr_init(struct cix_sp_crash_monitor *mdev)
{
	struct device *dev = mdev->dev;

	g_se_rdr_dump_mem_region.name = sky1_se_mem[0].name;
	g_se_rdr_dump_mem_region.length = sky1_se_mem[0].len;
	g_se_rdr_dump_mem_region.va = devm_ioremap_wc(mdev->dev, sky1_se_mem[0].da, sky1_se_mem[0].len);

	if (!g_se_rdr_dump_mem_region.va) {
		dev_err(dev, "failed to remap \n");
		return -ENOMEM;
	}

	dev_dbg(dev, "g_se_rdr_dump_mem_region name = %s, va = %px, len = %u\n",
			g_se_rdr_dump_mem_region.name, g_se_rdr_dump_mem_region.va, g_se_rdr_dump_mem_region.length);

	g_pm_rdr_dump_mem_region.name = sky1_pm_mem[0].name;
	g_pm_rdr_dump_mem_region.length = sky1_pm_mem[0].len;
	g_pm_rdr_dump_mem_region.va = devm_ioremap_wc(mdev->dev, sky1_pm_mem[0].da, sky1_pm_mem[0].len);

	if (!g_pm_rdr_dump_mem_region.va) {
		dev_err(dev, "failed to remap \n");
		return -ENOMEM;
	}

	dev_dbg(dev, "g_pm_rdr_dump_mem_region name = %s, va = %px, len = %u\n",
			g_pm_rdr_dump_mem_region.name, g_pm_rdr_dump_mem_region.va, g_pm_rdr_dump_mem_region.length);


	return 0;
}

static void cix_se_crash_work(struct work_struct *work)
{
	rdr_system_error(RDR_CSUSE_SOC_ISR_ERR_MODID, 0, 0);
}

static void cix_pm_crash_work(struct work_struct *work)
{
	rdr_system_error(RDR_CSUPM_SOC_ISR_ERR_MODID, 0, 0);
}

static int cix_sp_crash_m_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct cix_sp_crash_monitor *mdev;

	dev_dbg(dev, " %s\n", __func__);

	mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
	if (!mdev)
		return -ENOMEM;

	mdev->rx_channel = cix_sp_mbox_request_channel(pdev, "rx4");

	if (!mdev->rx_channel)
		return -EPROBE_DEFER;

	mdev->dev = &pdev->dev;
	platform_set_drvdata(pdev, mdev);

	spin_lock_init(&mdev->lock);

	if (mdev->rx_channel) {
		mdev->rx_buffer = devm_kzalloc(&pdev->dev,
					       MBOX_MAX_MSG_LEN, GFP_KERNEL);
		if (!mdev->rx_buffer)
			return -ENOMEM;
	}

	init_waitqueue_head(&mdev->waitq);
	dev_info(&pdev->dev, "Successfully registered\n");

	mdev->workqueue = create_workqueue(dev_name(dev));
	if (!mdev->workqueue) {
		dev_err(dev, "cannot create se workqueue\n");
		ret = -ENOMEM;
	}

	INIT_WORK(&mdev->se_work, cix_se_crash_work);

	INIT_WORK(&mdev->pm_work, cix_pm_crash_work);

    cix_sp_rproc_addr_init(mdev);

	rdr_sky1_se_register_exception();

	rdr_sky1_pm_register_exception();

	ret = rdr_sky1_sp_register_core();
	if (ret) {
		dev_err(dev, "rdr_sky1_sf_register_core fail, ret = [%d]\n", ret);
		goto err_register;
	}

	return 0;

err_register:
	destroy_workqueue(mdev->workqueue);

	return ret;
}

static int cix_sp_crash_m_remove(struct platform_device *pdev)
{
	struct cix_sp_crash_monitor *mdev = platform_get_drvdata(pdev);

	if (mdev->rx_channel)
		mbox_free_channel(mdev->rx_channel);

	rdr_sky1_sp_unregister_core();
	rdr_sky1_sp_unregister_exception();

	return 0;
}

static const struct of_device_id cix_sp_crash_m_of_match[] = {
	{ .compatible = "cix,se_pm_crash", .data = NULL, },
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, cix_sp_crash_m_of_match);


static struct platform_driver cix_sp_crash_m_driver = {
	.probe = cix_sp_crash_m_probe,
	.remove = cix_sp_crash_m_remove,
	.driver = {
		.name = "cix-se-pm-crash",
		.of_match_table = of_match_ptr(cix_sp_crash_m_of_match),
	},
};
module_platform_driver(cix_sp_crash_m_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CIX SE/PM crash monitor Driver");
MODULE_AUTHOR("Finn Zhang <Finn.Zhang@cixtech.com>");

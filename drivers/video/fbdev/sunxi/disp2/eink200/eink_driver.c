#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm_runtime.h>
#include <linux/poll.h>

#include "include/eink_driver.h"
#include "include/eink_sys_source.h"
#include "include/fmt_convert.h"
#include "eink_fbdev.h"
#include "hand_write.h"


struct disp_layer_config_inner eink_para[16];
struct disp_layer_config2 eink_lyr_cfg2[16];

static wait_queue_head_t eink_poll_queue;
struct mutex file_ops_lock;

struct eink_driver_info g_eink_drvdata;

u32 force_temp;
u32 force_fresh_mode;

struct timespec st_standby, et_standby;

static ssize_t eink_dbglvl_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	count += sprintf(buf, "\t0 -- no log\n");
	count += sprintf(buf + count, "\t9 -- capture the pic send to eink\n");
	count += sprintf(buf + count, "\t8 -- print timer info\n");
	count += sprintf(buf + count, "\t7 -- capture DE WB BUF\n");
	count += sprintf(buf + count, "\t6 -- capture wavfile\n");
	count += sprintf(buf + count, "\t5 -- decode or rmi debug/fresh img\n");
	count += sprintf(buf + count, "\t4 -- dump register\n");
	count += sprintf(buf + count, "\t3 -- print pipe and buf list\n");
	count += sprintf(buf + count, "\t2 -- debug log more\n");
	count += sprintf(buf + count, "\t1 -- base debug info\n");
	count += sprintf(buf + count, "\t\n");
	count += sprintf(buf + count, "\t[EINK]current debug level = %d\n", eink_get_print_level());

	return count;
}

static ssize_t eink_dbglvl_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	pr_info("[EINK]set debug level = %s\n", buf);

	if (buf[0] >= '0' && buf[0] <= '9') {
		eink_set_print_level(buf[0] - '0');
	} else {
		pr_info("please set level 0~9 range\n");
	}

	return count;
}

static ssize_t eink_upd_mode_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get upd_mode = 0x%x\n", force_fresh_mode);
}

static ssize_t eink_upd_mode_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long value = 0;
	int err = 0;

	pr_info("[EINK]set upd_mode = %s\n", buf);

	err = kstrtoul(buf, 0, &value);
	if (err) {
		pr_warn("[%s]Invalid size\n", __func__);
		return err;
	}
	force_fresh_mode = value;

	return count;
}

static ssize_t eink_temperature_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get temperture = %d\n", force_fresh_mode);
}

static ssize_t eink_temperature_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long value = 0;
	int err = 0;

	pr_info("[EINK]set temperature = %s\n", buf);

	err = kstrtoul(buf, 10, &value);
	if (err) {
		pr_warn("[%s]Invalid size\n", __func__);
		return err;
	}
	force_temp = value;

	return count;
}

static ssize_t eink_sys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct eink_manager *eink_mgr = NULL;
	ssize_t count = 0;
	u32 width = 0, height = 0;

	eink_mgr = get_eink_manager();
	if (eink_mgr == NULL)
		return 0;

	eink_mgr->get_resolution(eink_mgr, &width, &height);

	if (eink_mgr->enable_flag == false) {
		pr_warn("eink not enable yet!\n");
		return 0;
	}

	count += sprintf(buf + count, "eink_rate %d hz, panel_rate %d hz, fps:%d\n",
			eink_mgr->get_clk_rate(eink_mgr->ee_clk),
			eink_mgr->get_clk_rate(eink_mgr->panel_clk), eink_mgr->get_fps(eink_mgr));
	count += eink_mgr->dump_config(eink_mgr, buf + count);
	/* output */
	count += sprintf(buf + count,
			"\teink output\n");

	return count;
}

static ssize_t eink_sys_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static ssize_t eink_standby_cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	pr_info("[%s]:buf = %s, count = %d\n", __func__, buf, (unsigned int)count);
	if (!strncmp(buf, "suspend", (count - 1)))
		eink_suspend(g_eink_drvdata.device);
	if (!strncmp(buf, "resume", (count - 1)))
		eink_resume(g_eink_drvdata.device);

	return count;
}

static DEVICE_ATTR(dbglvl, 0660, eink_dbglvl_show, eink_dbglvl_store);
static DEVICE_ATTR(sys, 0660, eink_sys_show, eink_sys_store);
static DEVICE_ATTR(standby_cmd, 0660, NULL, eink_standby_cmd_store);
static DEVICE_ATTR(temperature, 0660, eink_temperature_show, eink_temperature_store);
static DEVICE_ATTR(upd_mode, 0660, eink_upd_mode_show, eink_upd_mode_store);

static struct attribute *eink_attributes[] = {
	&dev_attr_sys.attr,
	&dev_attr_dbglvl.attr,
	&dev_attr_standby_cmd.attr,
	&dev_attr_temperature.attr,
	&dev_attr_upd_mode.attr,
	NULL
};

static struct attribute_group eink_attribute_group = {
	.name = "attr",
	.attrs = eink_attributes
};

static int eink_get_clk(struct eink_driver_info *drvdata)
{
	int ret = 0;
	struct device *dev = drvdata->device;

	pr_info("[%s]dev = %lx\n", __func__, (unsigned long)dev);
	drvdata->init_para.de_clk = devm_clk_get(dev, "de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_clk)) {
		pr_err("get disp engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_clk);
		return ret;
	}

	drvdata->init_para.de_bus_clk = devm_clk_get(dev, "bus_de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_bus_clk)) {
		pr_err("get DE bus clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_bus_clk);
		return ret;
	}

	drvdata->init_para.de_rst_clk = devm_reset_control_get_shared(dev, "rst_bus_de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_rst_clk)) {
		pr_err("get DE reset clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_rst_clk);
		return ret;
	}

	drvdata->init_para.ee_clk = devm_clk_get(dev, "eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_clk)) {
		pr_err("get eink engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_clk);
		return ret;
	}

	drvdata->init_para.ee_bus_clk = devm_clk_get(dev, "bus_eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_bus_clk)) {
		pr_err("get eink bus clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_bus_clk);
		return ret;
	}

	drvdata->init_para.ee_rst_clk = devm_reset_control_get(dev, "rst_bus_eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_rst_clk)) {
		pr_err("get eink reset clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_rst_clk);
		return ret;
	}
	drvdata->init_para.panel_clk = devm_clk_get(dev, "eink_panel");
	if (IS_ERR_OR_NULL(drvdata->init_para.panel_clk)) {
		pr_err("get edma clk clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.panel_clk);
		return ret;
	}
	return 0;
}

static void eink_put_clk(struct eink_driver_info *drvdata)
{
	struct device *dev = drvdata->device;

	if (!IS_ERR_OR_NULL(drvdata->init_para.panel_clk))
		devm_clk_put(dev, drvdata->init_para.panel_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.ee_rst_clk))
		reset_control_put(drvdata->init_para.ee_rst_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.ee_bus_clk))
		devm_clk_put(dev, drvdata->init_para.ee_bus_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.ee_clk))
		devm_clk_put(dev, drvdata->init_para.ee_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.de_rst_clk))
		reset_control_put(drvdata->init_para.de_rst_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.de_bus_clk))
		devm_clk_put(dev, drvdata->init_para.de_bus_clk);
	if (!IS_ERR_OR_NULL(drvdata->init_para.de_clk))
		devm_clk_put(dev, drvdata->init_para.de_clk);
	return;
}

/* unload resources of eink device */
static void eink_unload_resource(struct eink_driver_info *drvdata)
{
	eink_put_clk(drvdata);

	if (drvdata->init_para.ee_reg_base)
		iounmap(drvdata->init_para.ee_reg_base);
	if (drvdata->init_para.de_reg_base)
		iounmap(drvdata->init_para.de_reg_base);
	return;
}

static int eink_init(struct init_para *para)
{
	eink_get_sys_config(para);
	fmt_convert_mgr_init(para);
	eink_mgr_init(para);
	return 0;
}

static void eink_exit(void)
{
/*
	fmt_convert_mgr_exit(para);
	eink_mgr_exit(para);
*/
	return;
}

//static u64 eink_dmamask = DMA_BIT_MASK(32);
static int eink_probe(struct platform_device *pdev)
{
	int ret = 0, counter = 0;
	struct device_node *node = pdev->dev.of_node;
	struct eink_driver_info *drvdata = NULL;

	if (!of_device_is_available(node)) {
		pr_err("EINK device is not configed!\n");
		return -ENODEV;
	}

	drvdata = &g_eink_drvdata;
	drvdata->device = &pdev->dev;

	eink_set_print_level(EINK_PRINT_LEVEL);
	/* reg_base */
	drvdata->init_para.ee_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.ee_reg_base) {
		pr_err("of_iomap eink reg base failed!\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	counter++;
	drvdata->init_para.de_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.de_reg_base) {
		pr_err("of_iomap de wb reg base failed\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	/* clk */
	ret = eink_get_clk(drvdata);
	if (ret)
		goto err_out;
	/* irq */
	counter = 0;
	drvdata->init_para.ee_irq_no = irq_of_parse_and_map(node, counter);
	if (!drvdata->init_para.ee_irq_no) {
		pr_err("get eink irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	counter++;

	drvdata->init_para.de_irq_no = irq_of_parse_and_map(node, counter);
	if (!drvdata->init_para.de_irq_no) {
		pr_err("get de irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

/*
	pdev->dev.dma_mask = &eink_dmamask;
	pdev->dev.coherent_dma_mask = eink_dmamask;
*/
	platform_set_drvdata(pdev, (void *)drvdata);

#if defined(CONFIG_ION_SUNXI) && defined(EINK_CACHE_MEM)
	init_eink_ion_mgr(&g_eink_drvdata.ion_mgr);
#endif

	eink_init(&drvdata->init_para);
	eink_fbdev_init(&pdev->dev);

	g_eink_drvdata.eink_mgr = get_eink_manager();

	init_waitqueue_head(&eink_poll_queue);
	mutex_init(&file_ops_lock);
	mutex_init(&g_eink_drvdata.lock);
	hand_write_init();

	pr_info("[EINK] probe finish!\n");

	return 0;

err_out:
	eink_unload_resource(drvdata);
	dev_err(&pdev->dev, "eink probe failed, errno %d!\n", ret);
	return ret;
}

static void eink_shutdown(struct platform_device *pdev)
{
	int ret = 0, sel = 0;
	bool empty = false;
	u32 not_empty_cnt = 0;
	struct buf_manager *buf_mgr = NULL;
	struct fmt_convert_manager *cvt_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL!\n", __func__);
		return;
	}
	pr_info("[%s]:\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->shutdown_state) {
		EINK_INFO_MSG("eink has already shutdown!\n");
		mutex_unlock(&mgr->standby_lock);
		return;
	}

	eink_fb_exit();

	buf_mgr = mgr->buf_mgr;
	cvt_mgr = mgr->convert_mgr;
	if ((buf_mgr == NULL) || (cvt_mgr == NULL)) {
		pr_err("[%s]:buf mgr or cvt mgr is NULL\n", __func__);
		mutex_unlock(&mgr->standby_lock);
		return;
	}

	do {
		empty = buf_mgr->is_upd_list_empty(buf_mgr) && buf_mgr->is_coll_list_empty(buf_mgr);
		if (empty == false) {
			if (not_empty_cnt == 2000) {
				buf_mgr->wait_for_newest_img_node(buf_mgr);
			}
			not_empty_cnt++;
		}
		msleep(5);
	} while ((empty == false) && (not_empty_cnt <= 2000));

	if (not_empty_cnt > 2000) {
		pr_warn("[%s]:wait for buf empty timeout! fresh newest pic\n", __func__);
	}

	eink_clk_disable(mgr);

	ret = cvt_mgr->disable(sel);
	if (ret) {
		pr_err("[%s]fail to disable converter(%d) when shutdown!\n", __func__, ret);
		mutex_unlock(&mgr->standby_lock);
		return;
	}
	ret = mgr->eink_mgr_disable(mgr);
	if (ret == 0) {
		mgr->shutdown_state = 1;
	} else {
		pr_warn("[%s]:fail to disable ee when shutdown\n", __func__);
	}

	mutex_unlock(&mgr->standby_lock);
	pr_info("[%s]:finish!\n", __func__);

	return;
}

static int eink_remove(struct platform_device *pdev)
{
	struct eink_driver_info *drvdata;

	dev_info(&pdev->dev, "%s\n", __func__);

	eink_shutdown(pdev);

	drvdata = platform_get_drvdata(pdev);
	if (drvdata != NULL) {
		eink_unload_resource(drvdata);
		platform_set_drvdata(pdev, NULL);
	} else
		pr_err("%s:drvdata is NULL!\n", __func__);

	pr_info("%s finish!\n", __func__);

	return 0;
}

static int eink_runtime_suspend(struct device *dev)
{
	int ret = 0, sel = 0;
	struct buf_manager *buf_mgr = NULL;
	struct fmt_convert_manager *cvt_mgr = NULL;
	struct index_manager *index_mgr = NULL;
	struct pipe_manager *pipe_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL!\n", __func__);
		return -1;
	}
	pr_info("[%s]:-----------------\n", __func__);

	if (eink_get_print_level() == 8) {
		getnstimeofday(&st_standby);
	}
	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state) {
		EINK_INFO_MSG("eink has already suspend!\n");
		mutex_unlock(&mgr->standby_lock);
		return 0;
	}
	mgr->suspend_state = 1;

	buf_mgr = mgr->buf_mgr;
	cvt_mgr = mgr->convert_mgr;
	index_mgr = mgr->index_mgr;
	pipe_mgr = mgr->pipe_mgr;
	if ((buf_mgr == NULL) || (cvt_mgr == NULL) || (index_mgr == NULL) || (pipe_mgr == NULL)) {
		pr_err("[%s]:buf mgr / cvt mgr / index mgr /pipe mgr is NULL\n", __func__);
		mutex_unlock(&mgr->standby_lock);
		return -1;
	}


	if (mgr->detect_fresh_task) {
		kthread_stop(mgr->detect_fresh_task);
		pr_info("[%s]:Stop eink update thread!\n", __func__);
		mgr->detect_fresh_task = NULL;
	}

	pipe_mgr->reset_all_pipe(pipe_mgr);
	index_mgr->reset_rmi(index_mgr, mgr->panel_info.bit_num);
	buf_mgr->reset_all(buf_mgr);
	if (mgr->get_temperature_task) {
		kthread_stop(mgr->get_temperature_task);
		EINK_INFO_MSG("stop get_temperature_task\n");
		mgr->get_temperature_task = NULL;
	}

	if (mgr->clk_en_flag)
		eink_clk_disable(mgr);

	ret = cvt_mgr->disable(sel);
	if (ret) {
		pr_err("[%s]fail to disable converter(%d) when suspend!\n", __func__, ret);
		mutex_unlock(&mgr->standby_lock);
		return ret;
	}

	ret = mgr->eink_mgr_disable(mgr);
	if (ret != 0) {
		pr_warn("[%s]:fail to disable ee when suspend\n", __func__);
	}
	mgr->ee_finish = 1;

	mutex_unlock(&mgr->standby_lock);

	if (eink_get_print_level() == 8) {
		getnstimeofday(&et_standby);
		pr_info("Eink suspend take %d ms\n",
				get_delt_ms_timer(st_standby, et_standby));
	}

	pr_info("[%s]:finish!\n", __func__);

	return ret;
}

static int eink_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct eink_manager *mgr = get_eink_manager();
	struct index_manager *index_mgr = NULL;

	if (mgr == NULL) {
		pr_err("[%s]:eink mgr is NULL\n", __func__);
		return -1;
	}

	index_mgr = mgr->index_mgr;
	if (index_mgr == NULL) {
		pr_err("[%s]:index mgr is NULL\n", __func__);
		return -1;
	}

	pr_info("[%s]:-----------------\n", __func__);

	if (eink_get_print_level() == 8) {
		getnstimeofday(&st_standby);
	}

	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state == 0) {
		mutex_unlock(&mgr->standby_lock);
		EINK_INFO_MSG("eink has already resume\n");
		return 0;
	}

	if (mgr->get_temperature_task == NULL) {
		mgr->get_temperature_task = kthread_create(eink_get_temperature_thread, (void *)mgr, "get temperature proc");
		if (IS_ERR_OR_NULL(mgr->get_temperature_task)) {
			pr_err("create getting temperature of eink thread fail!\n");
			ret = PTR_ERR(mgr->get_temperature_task);
			return ret;
		} else {
			ret = wake_up_process(mgr->get_temperature_task);
			EINK_INFO_MSG("resume get_temperature_task\n");
		}
	}

	ret = eink_clk_enable(mgr);
	index_mgr->set_rmi_addr(index_mgr);

	mgr->suspend_state = 0;

	if (mgr->detect_fresh_task == NULL) {
		mgr->detect_fresh_task = kthread_create(detect_fresh_thread, (void *)mgr, "eink_fresh_proc");
		if (IS_ERR_OR_NULL(mgr->detect_fresh_task)) {
			pr_err("%s:create fresh thread failed!\n", __func__);
			ret = PTR_ERR(mgr->detect_fresh_task);
			return ret;
		}

		wake_up_process(mgr->detect_fresh_task);
	}
	mutex_unlock(&mgr->standby_lock);

	if (eink_get_print_level() == 8) {
		getnstimeofday(&et_standby);
		pr_info("Eink resume take %d ms\n",
				get_delt_ms_timer(st_standby, et_standby));
	}

	pr_info("[%s]:finish!\n", __func__);

	return ret;
}

int eink_suspend(struct device *dev)
{
	int ret = 0;

	ret = eink_runtime_suspend(dev);

	if (ret < 0)
		pr_err("[%s]:failed!\n", __func__);

	return ret;
}

int eink_resume(struct device *dev)
{
	int ret = 0;
	pr_info("[%s]:start!\n", __func__);

	ret = eink_runtime_resume(dev);
	if (ret < 0) {
		pr_err("%s: failed!\n", __func__);
		return ret;
	}

	pr_info("[%s]:finish!\n", __func__);

	return ret;
}

int eink_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s open the device!\n", __func__);
	return ret;
}

int eink_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	pr_info("%s finish!\n", __func__);
	return ret;
}

bool eink_buf_msg_poll(void)
{
	struct eink_manager *eink_mgr = NULL;
	struct buf_manager *buf_mgr = NULL;

	eink_mgr = g_eink_drvdata.eink_mgr;
	if (eink_mgr == NULL) {
		return false;
	}

	buf_mgr = eink_mgr->buf_mgr;
	if (buf_mgr == NULL) {
		return false;
	}
	if (buf_mgr->get_buf_msg)
		return buf_mgr->get_buf_msg(buf_mgr);
	return false;
}

unsigned int eink_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	mutex_lock(&file_ops_lock);
	poll_wait(filp, &eink_poll_queue, wait);
	if (eink_buf_msg_poll())
		mask |= POLLIN | POLLRDNORM;
	mutex_unlock(&file_ops_lock);

	return mask;
}


long eink_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned long karg[4];
	unsigned long ubuffer[4] = { 0 };
	struct eink_manager *eink_mgr = NULL;

	eink_mgr = g_eink_drvdata.eink_mgr;

	if (copy_from_user
			((void *)karg, (void __user *)arg, 4 * sizeof(unsigned long))) {
		pr_err("[%s]copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	ubuffer[0] = *(unsigned long *)karg;
	ubuffer[1] = (*(unsigned long *)(karg + 1));
	ubuffer[2] = (*(unsigned long *)(karg + 2));
	ubuffer[3] = (*(unsigned long *)(karg + 3));

	switch (cmd) {
	case EINK_REGAL_PROCESS:
		{
			struct eink_img last_img;
			struct eink_img cur_img;
			if (copy_from_user(&last_img, (void __user *)ubuffer[0],
						sizeof(struct eink_img))) {
				pr_err("last img copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			if (copy_from_user(&cur_img, (void __user *)ubuffer[1],
						sizeof(struct eink_img))) {
				pr_err("cur img copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}
			ret = regal_eink_process(&cur_img, &last_img);
			break;
		}
	case EINK_UPDATE_IMG:
		{
			struct eink_upd_cfg eink_cfg;

			if (!eink_mgr) {
				pr_err("there is no eink manager!\n");
				break;
			}

			memset(&eink_cfg, 0, sizeof(struct eink_upd_cfg));
			if (copy_from_user(&eink_cfg, (void __user *)ubuffer[0],
						sizeof(struct eink_upd_cfg))) {
				pr_err("eink config copy_from_user fail\n");
				return -EFAULT;
			}

			if (eink_mgr->eink_update)
				ret = eink_mgr->eink_update(eink_mgr,
						(struct eink_upd_cfg *)&eink_cfg);
			break;
		}

	case EINK_WRITE_BACK_IMG:
		{
			s32 i = 0;
			struct eink_img last_img;
			struct eink_img cur_img;

			if (!eink_mgr) {
				pr_err("there is no eink manager!\n");
				break;
			}

			mutex_lock(&g_eink_drvdata.lock);
			memset(eink_lyr_cfg2, 0,
					16 * sizeof(struct disp_layer_config2));
			if (copy_from_user(eink_lyr_cfg2, (void __user *)ubuffer[0],
						sizeof(struct disp_layer_config2) * ubuffer[1])) {
				pr_err("layer config copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			if (copy_from_user(&last_img, (void __user *)ubuffer[2],
						sizeof(struct eink_img))) {
				pr_err("last img copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			if (copy_from_user(&cur_img, (void __user *)ubuffer[3],
						sizeof(struct eink_img))) {
				pr_err("cur img copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			for (i = 0; i < ubuffer[1]; i++)
				__disp_config2_transfer2inner(&eink_para[i],
						&eink_lyr_cfg2[i]);

			if (eink_mgr->eink_fmt_cvt_img)
				ret = eink_mgr->eink_fmt_cvt_img(
						(struct disp_layer_config_inner *)&eink_para[0],
						(unsigned int)ubuffer[1],
						&last_img, &cur_img);
			if (ret == 0) {
				if (copy_to_user((void __user *)ubuffer[3], &cur_img,
							sizeof(struct eink_img))) {
					pr_err("cur img copy_to_user fail\n");
					mutex_unlock(&g_eink_drvdata.lock);
					return -EFAULT;
				}
			}
			mutex_unlock(&g_eink_drvdata.lock);
			break;
		}

	case EINK_SET_TEMP:
		{
			if (eink_mgr->set_temperature)
				ret = eink_mgr->set_temperature(eink_mgr, ubuffer[0]);
			break;
		}

	case EINK_GET_TEMP:
		{
			if (eink_mgr->get_temperature)
				ret = eink_mgr->get_temperature(eink_mgr);
			break;
		}

	case EINK_GET_FREE_BUF:
		{
			struct buf_manager *buf_mgr;
			struct buf_slot slot;

			if (!eink_mgr) {
				pr_err("eink mgr is NULL!\n");
				return -EINVAL;
			}
			buf_mgr = eink_mgr->buf_mgr;
			if (!buf_mgr) {
				pr_err("buf mgr is NULL\n");
				return -EINVAL;
			}

			if (buf_mgr->get_free_buffer_slot)
				ret = buf_mgr->get_free_buffer_slot(buf_mgr, &slot);

			if (ret == 0) {
				if (copy_to_user((void __user *)ubuffer[0], &slot,
							sizeof(struct buf_slot))) {
					pr_err("buf slot copy_to_user fail\n");
					return -EFAULT;
				}
			}
			break;
		}

	case EINK_SET_GC_CNT:
		{
			if (eink_mgr->eink_set_global_clean_cnt)
				ret = eink_mgr->eink_set_global_clean_cnt(eink_mgr, ubuffer[0]);
			break;
		}

	case EINK_SELF_STANDBY:
		{
			if (g_eink_drvdata.device == NULL) {
				pr_err("Eink device is NULL! return\n");
				ret = -EINVAL;
				break;
			}
			EINK_INFO_MSG("[EINK_SELF_STANDBY]: %d\n", (int)ubuffer[0]);
			/* ubuffer[0] = 1 runtime_suspend, 0 runtime_resume */
			if (ubuffer[0]) {
				ret = eink_runtime_suspend(g_eink_drvdata.device);
			} else {
				ret = eink_runtime_resume(g_eink_drvdata.device);
			}

			if (ret < 0)
				pr_err("EINK_SELF_STANDBY : %d failed!\n", (int)ubuffer[0]);
			break;
		}

	case EINK_HANDWRITE_DMA_MAP:
		{
			u32 paddr = 0;
			mutex_lock(&g_eink_drvdata.lock);

			if (ubuffer[0] == 1) {
				ret = get_hand_write_phaddr(ubuffer[1], &paddr);
				if (ret) {
					pr_err("get_hand_write_phaddr fail!\n");
					mutex_unlock(&g_eink_drvdata.lock);
					return -EFAULT;
				}
				if (copy_to_user((void __user *)ubuffer[2], &paddr,
						 sizeof(u32))) {
					pr_err("cur img copy_to_user fail\n");
					mutex_unlock(&g_eink_drvdata.lock);
					return -EFAULT;
				}
			} else {
				release_hand_write_memory(ubuffer[1]);
			}

			mutex_unlock(&g_eink_drvdata.lock);
			break;
		}

	default:
		pr_err("The cmd is err!\n");
		break;
	}
	return ret;

}

#ifdef CONFIG_COMPAT
long eink_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	compat_uptr_t karg[4];
	unsigned long __user *ubuffer;

	if (copy_from_user
	    ((void *)karg, (void __user *)arg, 4 * sizeof(compat_uptr_t))) {
		__wrn("copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer = compat_alloc_user_space(4 * sizeof(unsigned long));
	if (!access_ok(VERIFY_WRITE, ubuffer, 4 * sizeof(unsigned long)))
		return -EFAULT;

	if (put_user(karg[0], &ubuffer[0]) ||
	    put_user(karg[1], &ubuffer[1]) ||
	    put_user(karg[2], &ubuffer[2]) || put_user(karg[3], &ubuffer[3])) {
		__wrn("put_user fail\n");
		return -EFAULT;
	}

	return eink_ioctl(file, cmd, (unsigned long)ubuffer);
}
#endif

const struct file_operations eink_fops = {
	.owner = THIS_MODULE,
	.open = eink_open,
	.release = eink_release,
	.poll = eink_poll,
	.unlocked_ioctl = eink_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = eink_compat_ioctl,
#endif
};

static const struct dev_pm_ops eink_pm_ops = {
	.suspend = eink_suspend,
	.resume = eink_resume,
};

static const struct of_device_id sunxi_eink_match[] = {
	{.compatible = "allwinner,sunxi-eink"},
	{},
};

static struct platform_driver eink_driver = {
	.probe = eink_probe,
	.remove = eink_remove,
	.shutdown = eink_shutdown,
	.driver = {
		.name = EINK_MODULE_NAME,
		.owner = THIS_MODULE,
		.pm = &eink_pm_ops,
		.of_match_table = sunxi_eink_match,
	},
};


static int __init eink_module_init(void)
{
	int ret = 0;
	struct eink_driver_info *drvdata = NULL;

	pr_info("[EINK] %s:\n", __func__);

	drvdata = &g_eink_drvdata;

	alloc_chrdev_region(&drvdata->devt, 0, 1, EINK_MODULE_NAME);
	drvdata->pcdev = cdev_alloc();
	cdev_init(drvdata->pcdev, &eink_fops);
	drvdata->pcdev->owner = THIS_MODULE;
	ret = cdev_add(drvdata->pcdev, drvdata->devt, 1);
	if (ret) {
		pr_err("eink cdev add major(%d) failed!\n", MAJOR(drvdata->devt));
		return -1;
	}
	drvdata->pclass = class_create(THIS_MODULE, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->pclass)) {
		pr_err("eink create class error!\n");
		ret = PTR_ERR(drvdata->pclass);
		return ret;
	}

	drvdata->eink_dev = device_create(drvdata->pclass, NULL,
			drvdata->devt, NULL, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->eink_dev)) {
		pr_err("eink device_create error!\n");
		ret = PTR_ERR(drvdata->eink_dev);
		return ret;
	}
	platform_driver_register(&eink_driver);

	ret = sysfs_create_group(&drvdata->eink_dev->kobj, &eink_attribute_group);
	if (ret < 0)
		pr_err("sysfs_create_file fail!\n");

	pr_info("[EINK] %s: finish\n", __func__);

	return 0;
}

static void __init eink_module_exit(void)
{
	struct eink_driver_info *drvdata = NULL;

	pr_info("[EINK]%s:\n", __func__);

	drvdata = &g_eink_drvdata;

	eink_exit();

#if defined(CONFIG_ION_SUNXI) && defined(EINK_CACHE_MEM)
	deinit_eink_ion_mgr(&g_eink_drvdata.ion_mgr);
#endif
	platform_driver_unregister(&eink_driver);
	device_destroy(drvdata->pclass, drvdata->devt);
	class_destroy(drvdata->pclass);
	cdev_del(drvdata->pcdev);
	unregister_chrdev_region(drvdata->devt, 1);
	return;
}

late_initcall(eink_module_init);
module_exit(eink_module_exit);

/* module_platform_driver(eink_driver); */

MODULE_DEVICE_TABLE(of, sunxi_eink_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liuli@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi Eink");

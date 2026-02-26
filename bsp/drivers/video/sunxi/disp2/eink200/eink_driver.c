/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#include "include/eink_driver.h"
#include "include/eink_sys_source.h"

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



struct disp_layer_config_inner eink_para[16];
struct disp_layer_config2 eink_lyr_cfg2[16];

static wait_queue_head_t eink_poll_queue;
struct mutex file_ops_lock;

struct eink_driver_info g_eink_drvdata;

u32 force_temp;

static ssize_t eink_debug_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%d\n", eink_get_print_level());
}

static ssize_t eink_debug_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	if (strncasecmp(buf, "1", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 1;
	else if (strncasecmp(buf, "0", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 0;
	else if (strncasecmp(buf, "2", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 2;
	else if (strncasecmp(buf, "3", 1) == 0)
		g_eink_drvdata.eink_dbg_info = 3;
	else
		sunxi_err(NULL, "Error input!\n");

	return count;
}

static ssize_t eink_dbglvl_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get debug level = %d\n", eink_get_print_level());
}

static ssize_t eink_dbglvl_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	sunxi_info(NULL, "[EINK]set debug level = %s\n", buf);

	if (buf[0] >= '0' && buf[0] <= '9') {
		eink_set_print_level(buf[0] - '0');
	} else {
		sunxi_info(NULL, "please set level 0~9 range\n");
	}

	return count;
}

static ssize_t eink_temperature_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "[EINK]get temperture = %d\n", force_temp);
}

static ssize_t eink_temperature_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long value = 0;
	int err = 0;

	sunxi_info(NULL, "[EINK]set temperature = %s\n", buf);

	err = kstrtoul(buf, 10, &value);
	if (err) {
		sunxi_warn(NULL, "[%s]Invalid size\n", __func__);
		return err;
	}
	force_temp = value;

	return count;
}

#if defined(__DISP_TEMP_CODE__)
static ssize_t eink_sys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct eink_manager *eink_mgr = NULL;
	struct disp_manager *disp_mgr = NULL;
	ssize_t count = 0;
	u32 width = 0, height = 0;
	int num_layers, layer_id;
	int num_chans, chan_id;

	eink_mgr = get_eink_manager();
	if (eink_mgr == NULL)
		return 0;
	disp_mgr = disp_get_layer_manager(0);
	if (disp_mgr == NULL)
		return 0;

	eink_mgr->get_resolution(eink_mgr, &width, &height);

	if (eink_mgr->enable_flag == false) {
		sunxi_warn(NULL, "eink not enable yet!\n");
		return 0;
	}

	count += sprintf(buf + count, "eink_rate %d hz, panel_rate %d hz, fps:%d\n",
			eink_mgr->get_clk_rate(eink_mgr->ee_clk),
			eink_mgr->get_clk_rate(eink_mgr->panel_clk), eink_mgr->get_fps(eink_mgr));
	count += eink_mgr->dump_config(eink_mgr, buf + count);
	/* output */
	count += sprintf(buf + count,
			"\teink output\n");

	num_chans = bsp_disp_feat_get_num_channels(0);

	/* layer info */
	for (chan_id = 0; chan_id < num_chans; chan_id++) {
		num_layers =
			bsp_disp_feat_get_num_layers_by_chn(0,
					chan_id);
		for (layer_id = 0; layer_id < num_layers; layer_id++) {
			struct disp_layer *lyr = NULL;
			struct disp_layer_config config;

			lyr = disp_get_layer(0, chan_id, layer_id);
			config.channel = chan_id;
			config.layer_id = layer_id;
			disp_mgr->get_layer_config(disp_mgr, &config, 1);
			if (lyr && (true == config.enable) && lyr->dump)
				count += lyr->dump(lyr, buf + count);
		}
	}
	return count;
}

static ssize_t eink_sys_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}
#endif

static ssize_t eink_standby_cmd_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	sunxi_info(NULL, "[%s]:buf = %s, count = %d\n", __func__, buf, (unsigned int)count);
	if (!strncmp(buf, "suspend", (count - 1)))
		eink_suspend(g_eink_drvdata.device);
	if (!strncmp(buf, "resume", (count - 1)))
		eink_resume(g_eink_drvdata.device);

	return count;
}

static DEVICE_ATTR(debug, 0660, eink_debug_show, eink_debug_store);
static DEVICE_ATTR(dbglvl, 0660, eink_dbglvl_show, eink_dbglvl_store);
/* static DEVICE_ATTR(sys, 0660, eink_sys_show, eink_sys_store); */
static DEVICE_ATTR(standby_cmd, 0660, NULL, eink_standby_cmd_store);
static DEVICE_ATTR(temperature, 0660, eink_temperature_show, eink_temperature_store);

static struct attribute *eink_attributes[] = {
	&dev_attr_debug.attr,
/* 	&dev_attr_sys.attr, */
	&dev_attr_dbglvl.attr,
	&dev_attr_standby_cmd.attr,
	&dev_attr_temperature.attr,
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

	sunxi_info(NULL, "[%s]dev = %lx\n", __func__, (unsigned long)dev);
	drvdata->init_para.de_clk = devm_clk_get(dev, "de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_clk)) {
		sunxi_err(NULL, "get disp engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_clk);
		return ret;
	}

	drvdata->init_para.de_bus_clk = devm_clk_get(dev, "bus_de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_bus_clk)) {
		sunxi_err(NULL, "get DE bus clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_bus_clk);
		return ret;
	}

	drvdata->init_para.de_rst_clk = devm_reset_control_get_shared(dev, "rst_bus_de0");
	if (IS_ERR_OR_NULL(drvdata->init_para.de_rst_clk)) {
		sunxi_err(NULL, "get DE reset clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.de_rst_clk);
		return ret;
	}

	drvdata->init_para.ee_clk = devm_clk_get(dev, "eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_clk)) {
		sunxi_err(NULL, "get eink engine clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_clk);
		return ret;
	}

	drvdata->init_para.ee_bus_clk = devm_clk_get(dev, "bus_eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_bus_clk)) {
		sunxi_err(NULL, "get eink bus clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_bus_clk);
		return ret;
	}

	drvdata->init_para.ee_rst_clk = devm_reset_control_get(dev, "rst_bus_eink");
	if (IS_ERR_OR_NULL(drvdata->init_para.ee_rst_clk)) {
		sunxi_err(NULL, "get eink reset clock failed!\n");
		ret = PTR_ERR(drvdata->init_para.ee_rst_clk);
		return ret;
	}
	drvdata->init_para.panel_clk = devm_clk_get(dev, "eink_panel");
	if (IS_ERR_OR_NULL(drvdata->init_para.panel_clk)) {
		sunxi_err(NULL, "get edma clk clock failed!\n");
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

/* static u64 eink_dmamask = DMA_BIT_MASK(32); */
static int eink_probe(struct platform_device *pdev)
{
	int ret = 0, counter = 0;
	struct device_node *node = pdev->dev.of_node;
	struct eink_driver_info *drvdata = NULL;

	if (!of_device_is_available(node)) {
		sunxi_err(NULL, "EINK device is not configed!\n");
		return -ENODEV;
	}

	drvdata = &g_eink_drvdata;

	drvdata->device = &pdev->dev;

	eink_set_print_level(EINK_PRINT_LEVEL);
	/* reg_base */
	drvdata->init_para.ee_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.ee_reg_base) {
		sunxi_err(NULL, "of_iomap eink reg base failed!\n");
		ret =  -ENOMEM;
		goto err_out;
	}
	counter++;
	drvdata->init_para.de_reg_base = of_iomap(node, counter);
	if (!drvdata->init_para.de_reg_base) {
		sunxi_err(NULL, "of_iomap de wb reg base failed\n");
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
		sunxi_err(NULL, "get eink irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	counter++;

	drvdata->init_para.de_irq_no = irq_of_parse_and_map(node, counter);
	if (!drvdata->init_para.de_irq_no) {
		sunxi_err(NULL, "get de irq failed!\n");
		ret = -EINVAL;
		goto err_out;
	}
/*
	pdev->dev.dma_mask = &eink_dmamask;
	pdev->dev.coherent_dma_mask = eink_dmamask;
*/
	platform_set_drvdata(pdev, (void *)drvdata);

#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
	init_eink_ion_mgr(&g_eink_drvdata.ion_mgr);
#endif

	eink_init(&drvdata->init_para);

	g_eink_drvdata.eink_mgr = get_eink_manager();

	init_waitqueue_head(&eink_poll_queue);
	mutex_init(&file_ops_lock);
	mutex_init(&g_eink_drvdata.lock);

	pm_runtime_set_active(drvdata->device);
	/* inc usercount so eink not runtime suspend */
	pm_runtime_get_noresume(drvdata->device);
	pm_runtime_set_autosuspend_delay(drvdata->device, 600);
	pm_runtime_use_autosuspend(drvdata->device);
	pm_runtime_enable(drvdata->device);
	device_enable_async_suspend(drvdata->device);

	sunxi_info(NULL, "[EINK] probe finish!\n");

	return 0;

err_out:
	eink_unload_resource(drvdata);
	sunxi_err(&pdev->dev, "eink probe failed, errno %d!\n", ret);
	return ret;
}

static void eink_shutdown(struct platform_device *pdev)
{
	int ret = 0;
	bool empty = false;
	u32 not_empty_cnt = 0;
	struct buf_manager *buf_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		sunxi_err(NULL, "[%s]:eink mgr is NULL!\n", __func__);
		return;
	}
	sunxi_info(NULL, "[%s]:\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->shutdown_state) {
		EINK_INFO_MSG("eink has already shutdown!\n");
		mutex_unlock(&mgr->standby_lock);
		return;
	}

	buf_mgr = mgr->buf_mgr;
	if (buf_mgr == NULL) {
		sunxi_err(NULL, "[%s]:buf mgr is NULL\n", __func__);
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
		sunxi_warn(NULL, "[%s]:wait for buf empty timeout! fresh newest pic\n", __func__);
	}

	eink_clk_disable(mgr);

	ret = mgr->eink_mgr_disable(mgr);
	if (ret == 0) {
		mgr->shutdown_state = 1;
	} else {
		sunxi_warn(NULL, "[%s]:fail to disable ee when shutdown\n", __func__);
	}

	mutex_unlock(&mgr->standby_lock);
	sunxi_info(NULL, "[%s]:finish!\n", __func__);

	return;
}

static int eink_remove(struct platform_device *pdev)
{
	struct eink_driver_info *drvdata;

	sunxi_info(&pdev->dev, "%s\n", __func__);

	eink_shutdown(pdev);

	pm_runtime_set_suspended(g_eink_drvdata.device);
	pm_runtime_dont_use_autosuspend(g_eink_drvdata.device);
	pm_runtime_disable(g_eink_drvdata.device);

	drvdata = platform_get_drvdata(pdev);
	if (drvdata != NULL) {
		eink_unload_resource(drvdata);
		platform_set_drvdata(pdev, NULL);
	} else
		sunxi_err(NULL, "%s:drvdata is NULL!\n", __func__);

	sunxi_info(NULL, "%s finish!\n", __func__);

	return 0;
}

static int eink_runtime_suspend(struct device *dev)
{
	int ret = 0;
	bool empty = false;
	u32 not_empty_cnt = 0;
	struct buf_manager *buf_mgr = NULL;
	struct eink_manager *mgr = get_eink_manager();

	if (mgr == NULL) {
		sunxi_err(NULL, "[%s]:eink mgr is NULL!\n", __func__);
		return -1;
	}
	sunxi_info(NULL, "[%s]:-----------------\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state) {
		EINK_INFO_MSG("eink has already suspend!\n");
		mutex_unlock(&mgr->standby_lock);
		return 0;
	}

	buf_mgr = mgr->buf_mgr;
	if (buf_mgr == NULL) {
		sunxi_err(NULL, "[%s]:buf mgr is NULL\n", __func__);
		mutex_unlock(&mgr->standby_lock);
		return -1;
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
		sunxi_warn(NULL, "[%s]:wait for buf empty timeout!fresh newest pic\n", __func__);
	}

#if defined(__DISP_TEMP_CODE__)
	if (mgr->detect_fresh_task) {
		kthread_stop(mgr->detect_fresh_task);
		sunxi_info(NULL, "[%s]:Stop eink update thread!\n", __func__);
		mgr->detect_fresh_task = NULL;
	}
#endif

	if (mgr->get_temperature_task) {
		kthread_stop(mgr->get_temperature_task);
		EINK_INFO_MSG("stop get_temperature_task\n");
		mgr->get_temperature_task = NULL;
	}

	if (mgr->clk_en_flag)
		eink_clk_disable(mgr);

	ret = mgr->eink_mgr_disable(mgr);
	if (ret == 0) {
		mgr->suspend_state = 1;
	} else {
		sunxi_warn(NULL, "[%s]:fail to disable ee when suspend\n", __func__);
	}

	mutex_unlock(&mgr->standby_lock);
	sunxi_info(NULL, "[%s]:finish!\n", __func__);

	return ret;
}

static int eink_runtime_idle(struct device *dev)
{
	return 0;
}

static int eink_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct eink_manager *mgr = get_eink_manager();
	struct index_manager *index_mgr = NULL;

	if (mgr == NULL) {
		sunxi_err(NULL, "[%s]:eink mgr is NULL\n", __func__);
		return -1;
	}

	index_mgr = mgr->index_mgr;
	if (index_mgr == NULL) {
		sunxi_err(NULL, "[%s]:index mgr is NULL\n", __func__);
		return -1;
	}

	sunxi_info(NULL, "[%s]:-----------------\n", __func__);

	mutex_lock(&mgr->standby_lock);
	if (mgr->suspend_state == 0) {
		mutex_unlock(&mgr->standby_lock);
		EINK_INFO_MSG("eink has already resume\n");
		return 0;
	}

	if (mgr->get_temperature_task == NULL) {
		mgr->get_temperature_task = kthread_create(eink_get_temperature_thread, (void *)mgr, "get temperature proc");
		if (IS_ERR_OR_NULL(mgr->get_temperature_task)) {
			sunxi_err(NULL, "create getting temperature of eink thread fail!\n");
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

#if defined(__DISP_TEMP_CODE__)
	if (mgr->detect_fresh_task == NULL) {
		mgr->detect_fresh_task = kthread_create(detect_fresh_thread, (void *)mgr, "eink_fresh_proc");
		if (IS_ERR_OR_NULL(mgr->detect_fresh_task)) {
			sunxi_err(NULL, "%s:create fresh thread failed!\n", __func__);
			ret = PTR_ERR(mgr->detect_fresh_task);
			return ret;
		}

		wake_up_process(mgr->detect_fresh_task);
	}
#endif
	mutex_unlock(&mgr->standby_lock);
	sunxi_info(NULL, "[%s]:finish!\n", __func__);

	return ret;
}

int eink_suspend(struct device *dev)
{
	int ret = 0;

	if (!pm_runtime_status_suspended(dev)) {
		eink_runtime_suspend(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_suspended(dev);
		pm_runtime_enable(dev);
	}

	sunxi_info(NULL, "[%s]:finish!\n", __func__);
	return ret;
}

int eink_resume(struct device *dev)
{
	int ret = 0;

	if (!pm_runtime_active(dev)) {
		eink_runtime_resume(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}

	sunxi_info(NULL, "[%s]:finish!\n", __func__);
	return ret;
}

int eink_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	sunxi_info(NULL, "%s open the device!\n", __func__);
	return ret;
}

int eink_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	sunxi_info(NULL, "%s finish!\n", __func__);
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

	EINK_INFO_MSG(" input !\n");
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
		sunxi_err(NULL, "copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer[0] = *(unsigned long *)karg;
	ubuffer[1] = (*(unsigned long *)(karg + 1));
	ubuffer[2] = (*(unsigned long *)(karg + 2));
	ubuffer[3] = (*(unsigned long *)(karg + 3));

	switch (cmd) {
	case EINK_UPDATE_IMG:
		{
			struct eink_upd_cfg eink_cfg;

			if (!eink_mgr) {
				sunxi_err(NULL, "there is no eink manager!\n");
				break;
			}

			memset(&eink_cfg, 0, sizeof(struct eink_upd_cfg));
			if (copy_from_user(&eink_cfg, (void __user *)ubuffer[0],
						sizeof(struct eink_upd_cfg))) {
				sunxi_err(NULL, "copy_from_user fail\n");
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
				sunxi_err(NULL, "there is no eink manager!\n");
				break;
			}

			mutex_lock(&g_eink_drvdata.lock);
			memset(eink_lyr_cfg2, 0,
					16 * sizeof(struct disp_layer_config2));
			if (copy_from_user(eink_lyr_cfg2, (void __user *)ubuffer[0],
						sizeof(struct disp_layer_config2) * ubuffer[1])) {
				sunxi_err(NULL, "copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			if (copy_from_user(&last_img, (void __user *)ubuffer[2],
						sizeof(struct eink_img))) {
				sunxi_err(NULL, "last img copy_from_user fail\n");
				mutex_unlock(&g_eink_drvdata.lock);
				return -EFAULT;
			}

			if (copy_from_user(&cur_img, (void __user *)ubuffer[3],
						sizeof(struct eink_img))) {
				sunxi_err(NULL, "cur img copy_from_user fail\n");
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
					sunxi_err(NULL, "cur img copy_to_user fail\n");
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
				sunxi_err(NULL, "eink mgr is NULL!\n");
				return -EINVAL;
			}
			buf_mgr = eink_mgr->buf_mgr;
			if (!buf_mgr) {
				sunxi_err(NULL, "buf mgr is NULL\n");
				return -EINVAL;
			}

			if (buf_mgr->get_free_buffer_slot)
				ret = buf_mgr->get_free_buffer_slot(buf_mgr, &slot);

			if (ret == 0) {
				if (copy_to_user((void __user *)ubuffer[0], &slot,
							sizeof(struct buf_slot))) {
					sunxi_err(NULL, "buf slot copy_to_user fail\n");
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
				sunxi_err(NULL, "Eink device is NULL! return\n");
				ret = -EINVAL;
				break;
			}
			/* ubuffer[0] = 1 runtime_suspend, 0 runtime_resume */
			if (ubuffer[0]) {
				pm_runtime_put(g_eink_drvdata.device);
				/* not need system suspend */
				/* before set status must disable runtime can effect */
				pm_runtime_disable(g_eink_drvdata.device);
				pm_runtime_set_suspended(g_eink_drvdata.device);
				pm_runtime_enable(g_eink_drvdata.device);
			} else {
				pm_runtime_get_sync(g_eink_drvdata.device);
				/* not need system resume */
				/* before set status must disable runtime can effect */
				pm_runtime_disable(g_eink_drvdata.device);
				pm_runtime_set_active(g_eink_drvdata.device);
				pm_runtime_enable(g_eink_drvdata.device);
			}
			break;
		}

	default:
		sunxi_err(NULL, "The cmd is err!\n");
		break;
	}
	return ret;

}

const struct file_operations eink_fops = {
	.owner = THIS_MODULE,
	.open = eink_open,
	.release = eink_release,
	.poll = eink_poll,
	.unlocked_ioctl = eink_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl = eink_ioctl,
#endif
};

static const struct dev_pm_ops eink_pm_ops = {
	.runtime_suspend = eink_runtime_suspend,
	.runtime_resume = eink_runtime_resume,
	.runtime_idle = eink_runtime_idle,
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

	sunxi_info(NULL, "[EINK] %s:\n", __func__);

	drvdata = &g_eink_drvdata;

	alloc_chrdev_region(&drvdata->devt, 0, 1, EINK_MODULE_NAME);
	drvdata->pcdev = cdev_alloc();
	cdev_init(drvdata->pcdev, &eink_fops);
	drvdata->pcdev->owner = THIS_MODULE;
	ret = cdev_add(drvdata->pcdev, drvdata->devt, 1);
	if (ret) {
		sunxi_err(NULL, "eink cdev add major(%d) failed!\n", MAJOR(drvdata->devt));
		return -1;
	}
	drvdata->pclass = class_create(THIS_MODULE, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->pclass)) {
		sunxi_err(NULL, "eink create class error!\n");
		ret = PTR_ERR(drvdata->pclass);
		return ret;
	}

	drvdata->eink_dev = device_create(drvdata->pclass, NULL,
			drvdata->devt, NULL, EINK_MODULE_NAME);
	if (IS_ERR(drvdata->eink_dev)) {
		sunxi_err(NULL, "eink device_create error!\n");
		ret = PTR_ERR(drvdata->eink_dev);
		return ret;
	}
	platform_driver_register(&eink_driver);

	ret = sysfs_create_group(&drvdata->eink_dev->kobj, &eink_attribute_group);
	if (ret < 0)
		sunxi_err(NULL, "sysfs_create_file fail!\n");

	sunxi_info(NULL, "[EINK] %s: finish\n", __func__);

	return 0;
}

static void __init eink_module_exit(void)
{
	struct eink_driver_info *drvdata = NULL;

	sunxi_info(NULL, "[EINK]%s:\n", __func__);

	drvdata = &g_eink_drvdata;

	eink_exit();

#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
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
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("liuli@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi Eink");

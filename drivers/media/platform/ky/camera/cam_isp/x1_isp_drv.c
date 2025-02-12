// SPDX-License-Identifier: GPL-2.0
/*
 * Description on this file
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#include "x1_isp_drv.h"
#include "x1_isp_pipe.h"
#include <cam_plat.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <media/v4l2-device.h>
#include <linux/dma-buf.h>

#define ISP_FUNC_CLK_FREQ (307200000)
#define ISP_AXI_CLK_FREQ (307200000)

struct isp_char_device g_isp_cdevice = { 0 };

struct x1isp_dev *g_isp_dev = NULL;
struct spm_camera_ispfirm_ops g_ispfirm_ops;

#define GET_ISP_DEV(isp_dev) (isp_dev = g_isp_dev)
#define SET_ISP_DEV(isp_dev) (g_isp_dev = isp_dev)

#define IS_COMBINATION_PIPE_MODE(mode, work_mode) {		\
		mode = ((ISP_WORK_MODE_HDR == work_mode)	\
			|| (ISP_WORK_MODE_RGBW == work_mode)	\
			|| (ISP_WORK_MODE_RGBIR == work_mode));	\
	}

int x1isp_vi_send_cmd(unsigned int cmd, void *cmd_payload, unsigned int payload_len);
int x1isp_irq_callback(int irq_num, void *irq_data, unsigned int data_len);

/*********************************export to outside********************************************/
int x1isp_dev_open(void)
{
	int ret = 0;
	struct x1isp_dev *isp_dev = NULL;
	struct isp_firm isp_vi_ops = { };
	struct v4l2_device *v4l2_dev = NULL;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	isp_dev->open_cnt++;
	if (1 == isp_dev->open_cnt) {
		v4l2_dev = plat_cam_v4l2_device_get();
		if (!v4l2_dev) {
			isp_log_err("isp get v4l2 device fail");
			return -ENODEV;
		}
		//register callback to vi modulej, just need register once.
		isp_vi_ops.frameinfo_size = 0;
		g_ispfirm_ops.send_cmd = x1isp_vi_send_cmd;
		g_ispfirm_ops.irq_callback = x1isp_irq_callback;
		isp_vi_ops.ispfirm_ops = &g_ispfirm_ops;
		//should use v4l2_subdev_notify instead, if isp registed as v4l2 subdev.
		v4l2_dev->notify(NULL, PLAT_SD_NOTIFY_REGISTER_ISPFIRM, &isp_vi_ops);
		isp_dev->vi_funs = isp_vi_ops.vi_ops;
		plat_cam_v4l2_device_put(v4l2_dev);

		pm_runtime_get_sync(&isp_dev->plat_dev->dev);
	}

	return ret;
}

int x1isp_dev_release(void)
{
	int ret = 0;
	struct x1isp_dev *isp_dev = NULL;
	struct v4l2_device *v4l2_dev = NULL;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	isp_dev->open_cnt--;
	if (0 == isp_dev->open_cnt) {
		v4l2_dev = plat_cam_v4l2_device_get();
		if (!v4l2_dev) {
			isp_log_err("isp get v4l2 device fail");
			return -ENODEV;
		}
		//should use v4l2_subdev_notify instead, if isp registed as v4l2 subdev.
		v4l2_dev->notify(NULL, PLAT_SD_NOTIFY_REGISTER_ISPFIRM, NULL);
		isp_dev->vi_funs = NULL;
		plat_cam_v4l2_device_put(v4l2_dev);

		if (atomic_read(&isp_dev->clk_ref)) {
			isp_log_warn("ispdev clks haven't been closed, now shutdown!");
			reset_control_assert(isp_dev->ahb_reset);
//			clk_disable_unprepare(isp_dev->ahb_clk);
			clk_disable_unprepare(isp_dev->fnc_clk);
			reset_control_assert(isp_dev->isp_reset);

			clk_disable_unprepare(isp_dev->axi_clk);
			reset_control_assert(isp_dev->isp_ci_reset);

			clk_disable_unprepare(isp_dev->dpu_clk);
			reset_control_assert(isp_dev->lcd_mclk_reset);
			atomic_set(&isp_dev->clk_ref, 0);
		}

		pm_runtime_put_sync(&isp_dev->plat_dev->dev);
	}

	return ret;
}

int x1isp_dev_clock_set(int enable)
{
	int ret = 0;
	// unsigned long clk_val = 0;
	struct x1isp_dev *isp_dev = NULL;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	if (0 == isp_dev->open_cnt) {
		isp_log_err("Please open ispdev first before operate clock!");
		return -1;
	}

	if (enable) {
		atomic_inc(&isp_dev->clk_ref);
		if (1 == atomic_read(&isp_dev->clk_ref)) {
			reset_control_deassert(isp_dev->ahb_reset);
//			clk_prepare_enable(isp_dev->ahb_clk);
			clk_prepare_enable(isp_dev->fnc_clk);
			reset_control_deassert(isp_dev->isp_reset);

			clk_prepare_enable(isp_dev->axi_clk);
			reset_control_deassert(isp_dev->isp_ci_reset);

			clk_prepare_enable(isp_dev->dpu_clk);
			reset_control_deassert(isp_dev->lcd_mclk_reset);
		}
	} else {
		atomic_dec(&isp_dev->clk_ref);
		if (0 == atomic_read(&isp_dev->clk_ref)) {
			reset_control_assert(isp_dev->ahb_reset);
//			clk_disable_unprepare(isp_dev->ahb_clk);
			clk_disable_unprepare(isp_dev->fnc_clk);
			reset_control_assert(isp_dev->isp_reset);

			clk_disable_unprepare(isp_dev->axi_clk);
			reset_control_assert(isp_dev->isp_ci_reset);

			clk_disable_unprepare(isp_dev->dpu_clk);
			reset_control_assert(isp_dev->lcd_mclk_reset);
		}
	}

	return ret;
}

int x1isp_dev_get_pipedev(u32 hw_pipe_id, struct x1isp_pipe_dev **pp_pipedev)
{
	struct x1isp_dev *isp_dev = NULL;
	u32 pipedev_id = hw_pipe_id;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	*pp_pipedev = isp_dev->pipe_devs[pipedev_id];

	return 0;
}

int x1isp_dev_get_vi_ops(struct spm_camera_vi_ops **pp_vi_ops)
{
	struct x1isp_dev *isp_dev = NULL;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	*pp_vi_ops = isp_dev->vi_funs;
	return 0;
}

int x1isp_dev_get_viraddr_from_dma_buf(struct dma_buf *dma_buffer, void **pp_vir_addr)
{
	int ret = 0;
	struct iosys_map map;

	ISP_DRV_CHECK_POINTER(dma_buffer);

	//dma-heap buffer
	if (dma_buffer->ops->begin_cpu_access) {
		ret = dma_buffer->ops->begin_cpu_access(dma_buffer, DMA_TO_DEVICE);
		if (ret < 0) {
			return -EPERM;
		}

		ret = dma_buffer->ops->vmap(dma_buffer, &map);
		if (0 == ret) {
			*pp_vir_addr = map.vaddr;
			isp_log_dbg("%s: get dma buf vir addr=0x%p!", __func__,
				    map.vaddr);
		} else {
			isp_log_info("%s: get dma buf vir addr failed!", __func__);
			ret = -EPERM;
		}
	} else {
		isp_log_err("%s: this dma buf has no begin_cpu_access function!",
			    __func__);
		ret = -EPERM;
	}

	return ret;
}

int x1isp_dev_put_viraddr_to_dma_buf(struct dma_buf *dma_buffer, void *vir_addr)
{
	struct iosys_map map;
	ISP_DRV_CHECK_POINTER(dma_buffer);
	{
		//dma-heap buffer
		map.vaddr = vir_addr;
		map.is_iomem = false;
		if (dma_buffer->ops->vunmap)
			dma_buffer->ops->vunmap(dma_buffer, &map);

		if (dma_buffer->ops->end_cpu_access)
			dma_buffer->ops->end_cpu_access(dma_buffer, DMA_TO_DEVICE);
	}

	return 0;
}

int _isp_dev_put_phyaddr_to_dma_buf(struct dma_buf *dma_buffer,
				    struct dma_buf_attachment *attach,
				    struct sg_table *sgt)
{
	dma_buf_unmap_attachment_unlocked(attach, sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(dma_buffer, attach);
	dma_buf_put(dma_buffer);

	return 0;
}

int x1isp_dev_get_phyaddr_from_dma_buf(int fd, __u64 *phy_addr)
{
	int ret = 0;
	struct dma_buf *dma_buffer = NULL;
	struct dma_buf_attachment *attach;
	struct x1isp_dev *isp_dev = NULL;
	struct sg_table *sgt;

	dma_buffer = dma_buf_get(fd);
	if (IS_ERR(dma_buffer)) {
		isp_log_err("%s: get dma buffer failed!", __func__);
		return -EBADF;
	}

	GET_ISP_DEV(isp_dev);
	attach = dma_buf_attach(dma_buffer, &isp_dev->plat_dev->dev);
	if (IS_ERR(attach)) {
		ret = -EPERM;
		goto fail_put;
	}

	get_dma_buf(dma_buffer);

	sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = -EPERM;
		goto fail_detach;
	}

	if (sgt->orig_nents != 1) {
		isp_log_err
		    ("%s: the count of sg table in this dma buffer isn't one, but %d!",
		     __func__, sgt->orig_nents);
		ret = -EPERM;
	} else {
		*phy_addr = sg_dma_address(sgt->sgl);
	}

	//we only get phy addr
	_isp_dev_put_phyaddr_to_dma_buf(dma_buffer, attach, sgt);
	dma_buf_put(dma_buffer);
	return ret;

fail_detach:
	dma_buf_detach(dma_buffer, attach);
	dma_buf_put(dma_buffer);
fail_put:
	dma_buf_put(dma_buffer);
	return ret;
}

/*********************************end export to outside********************************************/

#ifdef CONFIG_KY_DEBUG
struct dev_running_info {
	bool b_dev_running;
	bool (*is_dev_running)(struct dev_running_info *p_devinfo);
	struct notifier_block nb;
} isp_running_info;

static bool check_dev_running_status(struct dev_running_info *p_devinfo)
{
	return p_devinfo->b_dev_running;
}

#define to_devinfo(_nb) container_of(_nb, struct dev_running_info, nb)

static int dev_clkoffdet_notifier_handler(struct notifier_block *nb,
					  unsigned long msg, void *data)
{
	struct clk_notifier_data *cnd = data;
	struct dev_running_info *p_devinfo = to_devinfo(nb);

	if ((__clk_is_enabled(cnd->clk)) && (msg & PRE_RATE_CHANGE) &&
	    (cnd->new_rate == 0) && (cnd->old_rate != 0)) {
		if (p_devinfo->is_dev_running(p_devinfo))
			return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}
#endif

int x1isp_dev_context_create(struct platform_device *pdev)
{
	int ret = 0;
	struct x1isp_dev *isp_dev = NULL;

	//1. self struct initiate
	isp_dev = devm_kzalloc(&pdev->dev, sizeof(struct x1isp_dev), GFP_KERNEL);
	if (unlikely(isp_dev == NULL)) {
		dev_err(&pdev->dev, "could not allocate memory");
		ret = -ENOMEM;
		return ret;
	}

	isp_dev->plat_dev = pdev;

	//2. analyze dts and get reg addr base and length.
	/* get registers mem */
	isp_dev->isp_reg_source =
	    platform_get_resource_byname(pdev, IORESOURCE_MEM, "isp");
	if (!isp_dev->isp_reg_source) {
		dev_err(&pdev->dev, "no mem resource");
		ret = -ENODEV;
		return ret;
	}

	isp_dev->isp_regbase =
	    (ulong) devm_ioremap(&pdev->dev, isp_dev->isp_reg_source->start,
				 resource_size(isp_dev->isp_reg_source));
	if (IS_ERR((void *)isp_dev->isp_regbase)) {
		dev_err(&pdev->dev, "fail to remap iomem\n");
		ret = -EPERM;
		return ret;
	}
#ifdef CONFIG_ARCH_KY
	/* get clock(s) */
/*
	isp_dev->ahb_clk = devm_clk_get(&pdev->dev, "isp_ahb");
	if (IS_ERR(isp_dev->ahb_clk)) {
		ret = PTR_ERR(isp_dev->ahb_clk);
		dev_err(&pdev->dev, "failed to get ahb clock: %d\n", ret);
		return ret;
	}
*/
	isp_dev->ahb_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ahb_reset");
	if (IS_ERR_OR_NULL(isp_dev->ahb_reset)) {
		dev_err(&pdev->dev, "not found core isp_ahb_reset\n");
		return PTR_ERR(isp_dev->ahb_reset);
	}

	isp_dev->isp_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_reset");
	if (IS_ERR_OR_NULL(isp_dev->isp_reset)) {
		dev_err(&pdev->dev, "not found core isp_reset\n");
		return PTR_ERR(isp_dev->isp_reset);
	}

	isp_dev->isp_ci_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ci_reset");
	if (IS_ERR_OR_NULL(isp_dev->isp_ci_reset)) {
		dev_err(&pdev->dev, "not found core isp_ci_reset\n");
		return PTR_ERR(isp_dev->isp_ci_reset);
	}

	isp_dev->lcd_mclk_reset = devm_reset_control_get_optional_shared(&pdev->dev, "lcd_mclk_reset");
	if (IS_ERR_OR_NULL(isp_dev->lcd_mclk_reset)) {
		dev_err(&pdev->dev, "not found core lcd_mclk_reset\n");
		return PTR_ERR(isp_dev->lcd_mclk_reset);
	}

	isp_dev->fnc_clk = devm_clk_get(&pdev->dev, "isp_func");
	if (IS_ERR(isp_dev->fnc_clk)) {
		ret = PTR_ERR(isp_dev->fnc_clk);
		dev_err(&pdev->dev, "failed to get function clock: %d\n", ret);
		return ret;
	}
#ifdef CONFIG_KY_DEBUG
	isp_running_info.is_dev_running = check_dev_running_status;
	isp_running_info.nb.notifier_call = dev_clkoffdet_notifier_handler;
	clk_notifier_register(isp_dev->fnc_clk, &isp_running_info.nb);
#endif

	isp_dev->axi_clk = devm_clk_get(&pdev->dev, "isp_axi");
	if (IS_ERR(isp_dev->axi_clk)) {
		ret = PTR_ERR(isp_dev->axi_clk);
		dev_err(&pdev->dev, "failed to get bus clock: %d\n", ret);
		return ret;
	}

	isp_dev->dpu_clk = devm_clk_get(&pdev->dev, "dpu_mclk");
	if (IS_ERR(isp_dev->dpu_clk)) {
		ret = PTR_ERR(isp_dev->dpu_clk);
		dev_err(&pdev->dev, "failed to get dpu clock: %d\n", ret);
		return ret;
	}
#endif

	isp_dev->isp_regend =
	    isp_dev->isp_regbase + resource_size(isp_dev->isp_reg_source) - 1;
	x1isp_reg_set_base_addr(isp_dev->isp_regbase, isp_dev->isp_regend);

	ret = x1isp_pipe_dev_init(pdev, isp_dev->pipe_devs);
	if (ret) {
		dev_err(&pdev->dev, "pipedev init failed:%d!\n", ret);
		return ret;
	}

	init_completion(&isp_dev->reset_irq_complete);
	init_completion(&isp_dev->restart_complete);
	atomic_set(&isp_dev->clk_ref, 0);
	SET_ISP_DEV(isp_dev);

	return ret;
}

long x1isp_dev_copy_user(struct file *file, unsigned int cmd, void *arg,
			  x1isp_ioctl_func func)
{
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = arg;
	long err = -EINVAL;

	/*  Copy arguments into temp kernel buffer  */
	if (_IOC_DIR(cmd) != _IOC_NONE) {
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}

		err = -EFAULT;
		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			unsigned int n = _IOC_SIZE(cmd);

			if (copy_from_user(parg, (void __user *)arg, n))
				goto out;

			/* zero out anything we don't copy from userspace */
			if (n < _IOC_SIZE(cmd))
				memset((u8 *)parg + n, 0, _IOC_SIZE(cmd) - n);
		} else {
			/* read-only ioctl */
			memset(parg, 0, _IOC_SIZE(cmd));
		}
	}

	/* Handles IOCTL */
	err = func(file, cmd, (unsigned long)parg);
	if (err == -ENOIOCTLCMD)
		err = -ENOTTY;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		unsigned int n = _IOC_SIZE(cmd);
		if (copy_to_user((void __user *)arg, parg, n))
			goto out;
	}
out:
	if (mbuf) {
		kfree(mbuf);
		mbuf = NULL;
	}
	return err;
}

int x1isp_vi_send_cmd(unsigned int cmd, void *cmd_payload, unsigned int payload_len)
{
	int ret = 0;

	return ret;
}

int x1isp_dma_irq_handler(void *irq_data)
{
	struct x1isp_dev *isp_dev = NULL;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	int i;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	for (i = 0; i < ISP_PIPE_DEV_ID_MAX; i++) {
		pipe_dev = isp_dev->pipe_devs[i];
		x1isp_pipe_dma_irq_handler(pipe_dev, irq_data);
	}

	return 0;
}

/*
* the irq handler is registed in vi module,so isp just set callback to vi and called by vi. these happens in interrupt context.
*/
int x1isp_irq_callback(int irq_num, void *irq_data, unsigned int data_len)
{
	int ret = 0;

	if (ISP_IRQ == irq_num)
		x1isp_pipe_dev_irq_handler(irq_data);	/* isp irq */
	else if (DMA_IRQ == irq_num)
		x1isp_dma_irq_handler(irq_data);	/* dma irq */

	return ret;
}

int x1isp_cdevs_create(void)
{
	int ret = 0, cdev_count = 0, minor = ISP_PIPE_DEV_ID_0;
	dev_t devNum;
	int i = 0, devno = 0;
	struct cdev *cdev_array[ISP_PIPE_LINE_COUNT] = { NULL };
	struct device *device_array[ISP_PIPE_LINE_COUNT] = { NULL };
	struct file_operations *fops = NULL;

	//1. alloc cdev obj
	//two hardware pipelines + abstract pipes(1) + isp device
	memset(&g_isp_cdevice, 0, sizeof(struct isp_char_device));
	cdev_count = ISP_PIPE_LINE_COUNT;
	g_isp_cdevice.cdev_info =
	    (struct isp_cdev_info *)kzalloc(sizeof(struct isp_cdev_info) * cdev_count, GFP_KERNEL);
	if (NULL == g_isp_cdevice.cdev_info)
		return -ENOMEM;

	g_isp_cdevice.isp_cdev_cnt = cdev_count;
	ret = alloc_chrdev_region(&devNum, minor, cdev_count, X1_ISP_DEV_NAME);
	if (ret)
		goto ERR_STEP;

	g_isp_cdevice.cdev_num = devNum;
	g_isp_cdevice.cdev_major = MAJOR(devNum);

	g_isp_cdevice.isp_class = class_create(X1_ISP_DEV_NAME);
	if (IS_ERR(g_isp_cdevice.isp_class)) {
		ret = PTR_ERR(g_isp_cdevice.isp_class);
		goto ERR_STEP1;
	}

	fops = x1isp_pipe_get_fops();
	for (i = ISP_PIPE_DEV_ID_0; i < cdev_count; i++) {
		//2. init cdev obj
		cdev_array[i] = &g_isp_cdevice.cdev_info[i].isp_cdev;
		if (i <= ISP_HW_PIPELINE_ID_MAX) {
			//isp pipe dev
			cdev_init(cdev_array[i], fops);
		} else {
			//isp device
			// cdev_init(cdev_array[i], &g_isp_dev_fops);
		}
		cdev_array[i]->owner = THIS_MODULE;

		//3. register cdev obj
		devno = MKDEV(g_isp_cdevice.cdev_major, i);
		ret = cdev_add(cdev_array[i], devno, 1);
		if (ret) {
			cdev_array[i] = NULL;
			goto ERR_STEP2;
		}
		//4. create device
		if (i < ISP_HW_PIPELINE_ID_MAX) {
			//normal pipeline
			device_array[i] =
			    device_create(g_isp_cdevice.isp_class, NULL, devno, NULL,
					  "%s%d", X1_ISP_PIPE_DEV_NAME, i);
		}

		if (IS_ERR(device_array[i])) {
			ret = PTR_ERR(device_array[i]);
			device_array[i] = NULL;
			goto ERR_STEP2;
		}
	}

	return ret;

ERR_STEP2:
	for (i = ISP_PIPE_DEV_ID_0; i < cdev_count; i++) {
		if (cdev_array[i])
			cdev_del(cdev_array[i]);

		if (device_array[i])
			device_destroy(g_isp_cdevice.isp_class,
				       MKDEV(g_isp_cdevice.cdev_major, i));
	}
	class_destroy(g_isp_cdevice.isp_class);

ERR_STEP1:
	unregister_chrdev_region(devNum, cdev_count);

ERR_STEP:
	kfree(g_isp_cdevice.cdev_info);

	isp_log_err("%s : %s : %d - fail!", __FILE__, __func__, __LINE__);
	return ret;
}

void x1isp_cdevs_destroy(void)
{
	int i = 0;

	for (i = ISP_PIPE_DEV_ID_0; i < g_isp_cdevice.isp_cdev_cnt; i++) {
		device_destroy(g_isp_cdevice.isp_class,
			       MKDEV(g_isp_cdevice.cdev_major, i));
		cdev_del(&(g_isp_cdevice.cdev_info[i]).isp_cdev);
	}

	class_destroy(g_isp_cdevice.isp_class);
	unregister_chrdev_region(g_isp_cdevice.cdev_num, g_isp_cdevice.isp_cdev_cnt);
	kfree(g_isp_cdevice.cdev_info);
	g_isp_cdevice.cdev_info = NULL;
}

void x1isp_cdev_link_devices(void)
{
	int i = 0;
	struct x1isp_dev *isp_dev = NULL;

	GET_ISP_DEV(isp_dev);
	if (isp_dev && g_isp_cdevice.cdev_info) {
		for (i = ISP_PIPE_DEV_ID_0; i < g_isp_cdevice.isp_cdev_cnt; i++) {
			if (i < ISP_PIPE_DEV_ID_MAX)
				g_isp_cdevice.cdev_info[i].p_dev =
				    (void *)isp_dev->pipe_devs[i];	//pipe devices
			else
				g_isp_cdevice.cdev_info[i].p_dev = (void *)isp_dev; //isp device
		}
	}
}

static int x1isp_dev_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct x1isp_dev *isp_dev = NULL;

	isp_log_dbg("x1isp begin to probe");

	ret = x1isp_cdevs_create();
	if (ret) {
		isp_log_info("x1isp create cdevs fail!");
		return ret;
	}

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(33));
	ret = x1isp_dev_context_create(pdev);
	if (ret)
		goto ERR_STEP;

	x1isp_cdev_link_devices();
	/* enable runtime pm */
	pm_runtime_enable(&pdev->dev);

	GET_ISP_DEV(isp_dev);
	isp_log_dbg
	    ("x1isp device drvier probe successful,regBase=0x%lx,regEnd=0x%lx!",
	     isp_dev->isp_regbase, isp_dev->isp_regend);
	return 0;

ERR_STEP:
	x1isp_cdevs_destroy();
	isp_log_err("%s : %s : %d - fail!", __FILE__, __func__, __LINE__);
	return ret;
}

int x1isp_dev_context_destroy(struct platform_device *pdev)
{
	int ret = 0;
	struct x1isp_dev *isp_dev = NULL;

	GET_ISP_DEV(isp_dev);
	ISP_DRV_CHECK_POINTER(isp_dev);

	ret = x1isp_pipe_dev_exit(pdev, isp_dev->pipe_devs);

	/* put clock(s) */
#ifdef CONFIG_ARCH_KY
//	devm_clk_put(&pdev->dev, isp_dev->ahb_clk);
	devm_clk_put(&pdev->dev, isp_dev->axi_clk);

#ifdef CONFIG_KY_DEBUG
	clk_notifier_unregister(isp_dev->fnc_clk, &isp_running_info.nb);
#endif
	devm_clk_put(&pdev->dev, isp_dev->fnc_clk);
#endif

	// iounmap
	devm_iounmap(&pdev->dev, (void __iomem *)isp_dev->isp_regbase);
	//destory self struct
	devm_kfree(&pdev->dev, (void *)isp_dev);
	SET_ISP_DEV(NULL);

	return ret;
}

static int x1isp_dev_remove(struct platform_device *pdev)
{
	/* disable runtime pm */
	pm_runtime_disable(&pdev->dev);
	x1isp_dev_context_destroy(pdev);
	x1isp_cdevs_destroy();

	isp_log_dbg("x1isp device drvier remove successful!");
	return 0;
}


#ifdef CONFIG_PM_SLEEP
static int x1isp_dev_suspend(struct device *dev)
{
	/* TODO: */
	return 0;
}

static int x1isp_dev_resume(struct device *dev)
{
	/* TODO: */
	return 0;
}
#endif

#ifdef CONFIG_PM
static int x1isp_dev_runtime_suspend(struct device *dev)
{
	/* TODO: */
	return 0;
}

static int x1isp_dev_runtime_resume(struct device *dev)
{
	/* TODO: */
	return 0;
}
#endif

static const struct dev_pm_ops x1isp_pm_ops = {
	SET_RUNTIME_PM_OPS(x1isp_dev_runtime_suspend, x1isp_dev_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(x1isp_dev_suspend, x1isp_dev_resume)
};
static const struct of_device_id x1isp_dev_match[] = {
	{.compatible = "ky,x1isp" },
	{ },
};

MODULE_DEVICE_TABLE(of, x1isp_dev_match);

struct platform_driver x1isp_dev_driver = {
	.driver = {
		.name	 = X1_ISP_DEV_NAME,
		.of_match_table = of_match_ptr(x1isp_dev_match),
		.pm		= &x1isp_pm_ops,
	},
	.probe  = x1isp_dev_probe,
	.remove = x1isp_dev_remove,
};

module_platform_driver(x1isp_dev_driver);

MODULE_IMPORT_NS(DMA_BUF);
MODULE_AUTHOR("KY Inc.");
MODULE_DESCRIPTION("KY X1 ISP device driver");
MODULE_LICENSE("GPL");

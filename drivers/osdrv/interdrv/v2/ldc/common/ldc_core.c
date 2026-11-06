#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/compat.h>

#include <linux/comm_video.h>
#include <linux/ldc_uapi.h>

#include <base_cb.h>
#include <vi_sys.h>

#include "ldc_proc.h"
#include "ldc_debug.h"
#include "ldc_proc.h"
#include "ldc_sdk.h"
#include "ldc.h"

#define LDC_CLASS_NAME "soph-ldc"
#define LDC_DEV_NAME "soph-ldc"
#define LDC_CLK_NAME "ldc_clk"
#define LDC_CLK_SYS_NAME "clk_sys_3"
#define LDC0_REG_NAME "ldc0"
#define LDC1_REG_NAME "ldc1"
#define LDC2_REG_NAME "dwa0"
#define LDC3_REG_NAME "dwa1"
#define LDC0_INTR_NAME "ldc0"
#define LDC1_INTR_NAME "ldc1"
#define LDC2_INTR_NAME "dwa0"
#define LDC3_INTR_NAME "dwa1"

#define LDC_SHARE_MEM_SIZE (0x8000)

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#ifndef CONFIG_PROC_FS
#define CONFIG_PROC_FS 1
#endif

#ifndef CONFIG_PM_SLEEP
#define CONFIG_PM_SLEEP 1
#endif

static atomic_t ldc_open_count = ATOMIC_INIT(0);
static atomic_t suspend_flag = ATOMIC_INIT(0);

static struct ldc_vdev *ldc_dev;
static struct fasync_struct *ldc_fasync;

unsigned int ldc_log_lv = DBG_WARN;
bool ldc_dump_reg;
module_param(ldc_log_lv, int, 0644);
MODULE_PARM_DESC(ldc_log_lv, "LDC Debug Log Level");

static unsigned int clk_sys_freq[LDC_DEV_MAX_CNT];
static unsigned int core_cnt;
module_param_array(clk_sys_freq, int, &core_cnt, S_IRUGO);
MODULE_PARM_DESC(clk_sys_freq, "clk_sys_freq setting by user");

module_param(ldc_dump_reg, bool, 0644);
MODULE_PARM_DESC(ldc_dump_reg, "ldc need dump reg");

bool is_ldc_suspended(void)
{
	return atomic_read(&suspend_flag) == 1 ? true : false;
}

struct ldc_vdev *ldc_get_dev(void)
{
	return ldc_dev;
}

struct fasync_struct *ldc_get_dev_fasync(void)
{
	return ldc_fasync;
}

void ldc_core_init(int top_id)
{
	ldc_reset(top_id);
	ldc_init(top_id);
	ldc_intr_ctrl(0x00, top_id);//0x7 if you want get mesh tbl id err status
}

void ldc_core_deinit(int top_id)
{
	ldc_intr_ctrl(0x00, top_id);
	ldc_disable(top_id);
	ldc_reset(top_id);
}

void ldc_dev_init(struct ldc_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		atomic_set(&dev->core[i].state, LDC_CORE_STATE_IDLE);
		dev->core[i].dev_type = (enum ldc_type)i;
		ldc_core_init(i);
	}

	atomic_set(&dev->state, LDC_DEV_STATE_STOP);
}

void ldc_dev_deinit(struct ldc_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		atomic_set(&dev->core[i].state, LDC_CORE_STATE_IDLE);
		ldc_core_deinit(i);
	}

	atomic_set(&dev->state, LDC_DEV_STATE_STOP);
}

void ldc_enable_dev_clk(int coreid, bool en)
{
	struct ldc_vdev *dev = ldc_get_dev();

	if (!dev || !dev->core[coreid].clk_ldc) {
		TRACE_LDC(DBG_ERR, "null dev or null clk_ldc[%d]\n", coreid);
		return;
	}

	if (en) {
		if (!__clk_is_enabled(dev->core[coreid].clk_ldc))
			clk_enable(dev->core[coreid].clk_ldc);
	} else {
		if (__clk_is_enabled(dev->core[coreid].clk_ldc))
			clk_disable(dev->core[coreid].clk_ldc);
	}
}

static int ldc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ldc_vdev *wdev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = wdev->shared_mem;

	if ((vm_size + offset) > LDC_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE,
				    vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("ldc proc mmap vir(%p) phys(%#llx)\n", pos,
			 (unsigned long long)virt_to_phys((void *)pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long ldc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ldc_vdev *wdev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, drv_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(cmd);
	out_size = _IOC_SIZE(cmd);
	in_size = out_size;
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size))
		TRACE_LDC(DBG_ERR, "access_ok failed\n");
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size))
		TRACE_LDC(DBG_ERR, "access_ok failed\n");
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		TRACE_LDC(DBG_ERR, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	//TRACE_LDC(DBG_ERR, "ksize-in_size-cmd[%d-%d-%d]\n", ksize, in_size, cmd);
	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
		case LDC_INIT:
			break;
		case LDC_DEINIT:
			break;
		case LDC_BEGIN_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_BEGIN_JOB\n");
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

			ret = ldc_begin_job(wdev, data);
			break;
		}
		case LDC_END_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_END_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

			ret = ldc_end_job(wdev, data->handle);
			break;
		}
		case LDC_CANCEL_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_CANCLE_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

			ret = ldc_cancel_job(wdev, data->handle);
			break;
		}
		case LDC_GET_WORK_JOB: {
			struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

			CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

			ret = ldc_get_work_job(wdev, data);
			TRACE_LDC(DBG_DEBUG, "LDC_GET_WORK_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			break;
		}
		case LDC_ADD_ROT_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_ROT_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_rotation_task(wdev, attr);
			break;
		}
		case LDC_ADD_LDC_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_LDC_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_ldc_task(wdev, attr);
			break;
		}
		case LDC_ADD_COR_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_COR_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_cor_task(wdev, attr);
			break;
		}
		case LDC_ADD_WAR_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_WAR_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_warp_task(wdev, attr);
			break;
		}
		case LDC_ADD_AFF_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_AFF_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_affine_task(wdev, attr);
			break;
		}
		case LDC_ADD_LDC_LDC_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_LDC_LDC_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_ldc_ldc_task(wdev, attr);
			break;
		}
		case LDC_ADD_DWA_ROT_TASK: {
			struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_ADD_DWA_ROT_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

			ret = ldc_add_dwa_rot_task(wdev, attr);
			break;
		}
		case LDC_SET_JOB_IDENTITY: {
			struct gdc_identity_attr *identity = (struct gdc_identity_attr *)kdata;

			TRACE_LDC(DBG_DEBUG, "LDC_SET_JOB_IDENTITY, handle=0x%llx\n",
					  (unsigned long long)identity->handle);
			CHECK_IOCTL_CMD(cmd, struct gdc_identity_attr);

			ret = ldc_set_identity(wdev, identity);
			break;
		}
		case LDC_GET_CHN_FRM: {
			struct gdc_chn_frm_cfg *cfg = (struct gdc_chn_frm_cfg *)kdata;
			video_frame_info_s *video_frame = &cfg->video_frame;
			struct gdc_identity_attr *identity = &cfg->identity;
			int milli_sec = cfg->milli_sec;

			CHECK_IOCTL_CMD(cmd, struct gdc_chn_frm_cfg);

			ret = ldc_get_chn_frame(wdev, identity, video_frame, milli_sec);
			break;
		}
		// case LDC_SET_BUF_WRAP: {
		// 	TRACE_LDC(DBG_NOTICE, "LDC_SET_BUF_WRAP not support\n");
		// 	break;
		// }
		// case LDC_GET_BUF_WRAP: {
		// 	TRACE_LDC(DBG_NOTICE, "LDC_GET_BUF_WRAP not support\n");
		// 	break;
		// }
		case LDC_ATTACH_VB_POOL: {
			struct ldc_vb_pool_cfg *cfg = (struct ldc_vb_pool_cfg *)kdata;
			vb_pool pool = (vb_pool)cfg->vb_pool;

			CHECK_IOCTL_CMD(cmd, struct ldc_vb_pool_cfg);

			ret = ldc_attach_vb_pool(pool);
			break;
		}
		case LDC_DETACH_VB_POOL: {
			ret = ldc_detach_vb_pool();
			break;
		}
		case LDC_SUSPEND: {
			struct ldc_vdev *dev
				= container_of(filp->private_data, struct ldc_vdev, miscdev);

			ret = ldc_suspend(dev->miscdev.this_device);
			break;
		}
		case LDC_RESUME: {
			struct ldc_vdev *dev
				= container_of(filp->private_data, struct ldc_vdev, miscdev);

			ret = ldc_resume(dev->miscdev.this_device);
			break;
		}
		default:
			ret = -ENOTTY;
			goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);
	//TRACE_LDC(DBG_ERR, "kdata-stack_kdata[%px-%px]\n", kdata, stack_kdata);

	return ret;
}

static int ldc_open(struct inode *inode, struct file *filp)
{
	struct ldc_vdev *dev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("cannot find ldc private data\n");
		return -ENODEV;
	}

	i = atomic_inc_return(&ldc_open_count);
	if (i > 1) {
		pr_info("ldc_open: open %d times\n", i);
		return 0;
	}

	TRACE_LDC(DBG_INFO, "ldc_open\n");

	return 0;
}

static int ldc_release(struct inode *inode, struct file *filp)
{
	struct ldc_vdev *dev
		= container_of(filp->private_data, struct ldc_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("Cannot find stitch private data\n");
		return -ENODEV;
	}

	i = atomic_dec_return(&ldc_open_count);
	if (i) {
		pr_info("ldc_close: open %d times\n", i);
		return 0;
	}

	TRACE_LDC(DBG_INFO, "ldc_release.\n");
	return 0;
}

static int ldc_drv_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &ldc_fasync) >= 0)
		return 0;
	else
		return -EIO;
}

#ifdef CONFIG_COMPAT
static long ldc_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations ldc_fops = {
	.owner = THIS_MODULE,
	.open = ldc_open,
	.release = ldc_release,
	.mmap = ldc_mmap,
	.unlocked_ioctl = ldc_ioctl,
	.fasync  = ldc_drv_fasync,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ldc_compat_ptr_ioctl,
#endif
};

static void ldc_tsk_finish(struct ldc_vdev *dev, int top_id)
{
	struct ldc_core *core = &dev->core[top_id];
	ldc_wkup_frm_done_work(core);
}

#if LDC_USE_THREADED_IRQ
static irqreturn_t ldc_tsk_finish_thread_func(int irq, void *data)
{
	struct ldc_vdev *dev = (struct ldc_job *)data;
	struct ldc_core *core;
	int i, top_id = -1;
	struct ldc_job *done_job;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < dev->core_num; i++) {
		core = &dev->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			break;
		}
	}

	if (top_id >= 0 && top_id < dev->core_num)
		ldc_tsk_finish(dev, top_id);
	else
		TRACE_LDC(DBG_ERR, "invalid core[%d]\n", top_id);
	return IRQ_HANDLED;
}
#endif

static void ldc_irq_handler(unsigned char intr_status, struct ldc_vdev *dev, int top_id, struct ldc_job *done_job)
{
	if (!dev || !done_job)
		return;

	if (top_id < 0 || top_id >= LDC_DEV_MAX_CNT) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev_%d\n", top_id);
		return;
	}

	if (done_job->use_cmdq) {
		TRACE_LDC(DBG_DEBUG, "core(%d) cmdq status(%#x)\n", top_id, intr_status);
	} else {
		if (!(intr_status & BIT(0))) {
			TRACE_LDC(DBG_ERR, "core(%d) axi read err, status(%#x)\n", top_id, intr_status);
			return;
		}
	}

	atomic_set(&dev->core[top_id].state, LDC_CORE_STATE_END);
	TRACE_LDC(DBG_INFO, "core(%d) state set(%d)\n", top_id, LDC_CORE_STATE_END);

#if !LDC_USE_THREADED_IRQ
	ldc_tsk_finish(dev, top_id);
#endif
}

static irqreturn_t ldc_isr(int irq, void *data)
{
	struct ldc_vdev *dev = (struct ldc_vdev *)data;
	struct ldc_core *core;
	int top_id, i;
	unsigned char intr_status;
	struct ldc_job *done_job;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < dev->core_num; i++) {
		core = &dev->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			spin_lock(&core->core_lock);
			done_job = list_first_entry_or_null(&core->list, struct ldc_job, node);
			spin_unlock(&core->core_lock);

			if (unlikely(!done_job)) {
				TRACE_LDC(DBG_NOTICE, "null done job\n");
				goto CMDQ_STATUS_UNKOWN;
			}

			if (unlikely(done_job->coreid != top_id)) {
				TRACE_LDC(DBG_NOTICE, "done job core[%d] not match with [%d]\n", done_job->coreid, top_id);
				goto CMDQ_STATUS_UNKOWN;
			}

			if (done_job->use_cmdq) {
				intr_status = ldc_cmdq_intr_status(top_id);
				if (intr_status) {
					ldc_cmdq_intr_clr(top_id, intr_status);
					TRACE_LDC(DBG_DEBUG, "ldc_%d irqn(%d) cmdq-status(0x%x)\n"
						, top_id, irq, intr_status);

					if ((intr_status & 0x04) && ldc_cmdq_is_sw_restart(top_id)) {
						TRACE_LDC(DBG_DEBUG, "cmdq-sw restart\n");
						ldc_cmdq_sw_restart(top_id);
					}
				} else {
					goto CMDQ_STATUS_UNKOWN;
				}
			} else {
CMDQ_STATUS_UNKOWN:
				intr_status = ldc_intr_status(top_id);
				TRACE_LDC(DBG_INFO, "ldc_%d, irq(%d), status(0x%x)\n"
					, top_id, irq, intr_status);
				ldc_intr_clr(intr_status, top_id);
			}
			ldc_intr_ctrl(0x00, top_id);
			ldc_disable(top_id);

			ldc_irq_handler(intr_status, dev, top_id, done_job);
		}
	}
#if LDC_USE_THREADED_IRQ
	return IRQ_WAKE_THREAD;
#else
	return IRQ_HANDLED;
#endif
}

static int ldc_register_dev(struct ldc_vdev *wdev)
{
	int rc;

	wdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	wdev->miscdev.name = LDC_DEV_NAME;
	wdev->miscdev.fops = &ldc_fops;

	rc = misc_register(&wdev->miscdev);
	if (rc) {
		pr_err("ldc: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int ldc_init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	int i;
	int irq_num[LDC_DEV_MAX_CNT];
	const char * const irq_name[LDC_DEV_MAX_CNT] = {LDC0_INTR_NAME, LDC1_INTR_NAME, LDC2_INTR_NAME, LDC3_INTR_NAME};
	const char ldc_clk_name[LDC_DEV_MAX_CNT][16] = {"clk_ldc0", "clk_ldc1", "clk_dwa0", "clk_dwa1"};
	const char ldc_clk_sys_name[LDC_DEV_MAX_CNT][16] = {"clk_sys_3", "clk_sys_3", "clk_sys_3", "clk_sys_3"};

	struct resource *res[LDC_DEV_MAX_CNT];
	void __iomem *reg_base[LDC_DEV_MAX_CNT];
	struct ldc_vdev *dev;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get ldc drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(reg_base); ++i) {
		res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (unlikely(res[i] == NULL)) {
			dev_err(&pdev->dev, "invalid resource\n");
			return -EINVAL;
		}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res[i]->start,
			res[i]->end - res[i]->start);
		//reg_base[i] = devm_ioremap_resource(&pdev->dev, res[i]);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res[i]->start,
						res[i]->end - res[i]->start);
#endif
		TRACE_LDC(DBG_DEBUG, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px)\n"
			, i, res[i]->start, res[i]->end, reg_base[i]);
		ldc_set_base_addr(reg_base[i], i);

		irq_num[i] = platform_get_irq_byname(pdev, irq_name[i]);
		if (irq_num[i] < 0) {
			dev_err(&pdev->dev, "(%d)No IRQ resource for %s\n", i, irq_name[i]);
			return -ENODEV;
		}

		TRACE_LDC(DBG_DEBUG, "(%d)irq(%d) for %s get from platform driver.\n"
			, i, irq_num[i], irq_name[i]);

		//clk res
		dev->core[i].clk_src = devm_clk_get(&pdev->dev, ldc_clk_sys_name[i]);
		if (IS_ERR(dev->core[i].clk_src)) {
			TRACE_LDC(DBG_ERR, "Cannot get clk for clk_sys_3 for ldc[%d]\n", i);
			dev->core[i].clk_src = NULL;
		}
		dev->core[i].clk_ldc = devm_clk_get(&pdev->dev, ldc_clk_name[i]);
		if (IS_ERR(dev->core[i].clk_ldc)) {
			TRACE_LDC(DBG_ERR, "Cannot get clk for clk_ldc[%d]\n", i);
			dev->core[i].clk_ldc = NULL;
		}
		if (clk_sys_freq[i])
			dev->core[i].clk_sys_freq = clk_sys_freq[i];

#if LDC_USE_WORKQUEUE
		if (devm_request_irq(&pdev->dev, irq_num[i], ldc_isr, IRQF_SHARED
#elif LDC_USE_THREADED_IRQ
		if (devm_request_threaded_irq(&pdev->dev, irq_num[i], ldc_isr, ldc_tsk_finish_thread_func, IRQF_SHARED
#else
		if (devm_request_irq(&pdev->dev, irq_num[i], ldc_isr, IRQF_SHARED
#endif
			, irq_name[i], (void*)dev)) {
			dev_err(&pdev->dev, "Unable to request ldc_%d IRQ(%d)\n", i, irq_num[i]);
			return -EINVAL;
		}

		dev->core[i].irq_num = irq_num[i];
	}

	// clk_ldc_src_sel default 1(clk_src_vip_sys_2), 600 MHz
	// set 0(clk_src_vip_sys_4), 400MHz for ND
#if defined(__CV186X__)
		/*vi_sys_reg_write_mask(VI_SYS_CLK_LDC0_SRC_SEL,
					VI_SYS_CLK_LDC0_SRC_SEL_MASK,
					0 << VI_SYS_CLK_LDC0_SRC_SEL_OFFSET);
		*/
#else
		vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL1, BIT(20), 0);
#endif

#else
	TRACE_LDC(DBG_INFO, "cannot get res for stitch\n");
	rc = -1;
#endif

	return rc;
}

void ldc_clk_init(struct ldc_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}
	if (atomic_read(&dev->clk_en))
		return;
	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (dev->core[i].clk_src)
			clk_prepare_enable(dev->core[i].clk_src);
		if (dev->core[i].clk_apb)
			clk_prepare_enable(dev->core[i].clk_apb);
		if (dev->core[i].clk_ldc && !__clk_is_enabled(dev->core[i].clk_ldc))
			clk_prepare_enable(dev->core[i].clk_ldc);

		ldc_enable_dev_clk(i, true);

		if (clk_sys_freq[i])
			dev->core[i].clk_sys_freq = clk_sys_freq[i];
	}
	//TRACE_LDC(DBG_WARN, "ldc_clk_init\n");
	atomic_set(&dev->clk_en, true);
}

void ldc_clk_deinit(struct ldc_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}
	if (!atomic_read(&dev->clk_en))
		return;
	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (dev->core[i].clk_ldc && __clk_is_enabled(dev->core[i].clk_ldc))
			clk_disable_unprepare(dev->core[i].clk_ldc);
		if (dev->core[i].clk_apb)
			clk_disable_unprepare(dev->core[i].clk_apb);
		if (dev->core[i].clk_src)
			clk_disable_unprepare(dev->core[i].clk_src);

		if (clk_sys_freq[i]) {
			dev->core[i].clk_sys_freq = clk_sys_freq[i];
			clk_sys_freq[i] = 0;
		}
	}
	//TRACE_LDC(DBG_WARN, "ldc_clk_deinit\n");
	atomic_set(&dev->clk_en, false);
}

static int ldc_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct ldc_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("ldc cannot get drv data for ldc\n");
		return -EINVAL;
	}

	if (ldc_register_dev(wdev))
		goto err_dev;

#ifdef CONFIG_PROC_FS
	wdev->shared_mem = kzalloc(sizeof(struct ldc_proc_ctx), GFP_ATOMIC);
	if (!wdev->shared_mem) {
		pr_err("ldc shared mem alloc fail\n");
		rc = -ENOMEM;
		goto err_proc;
	}

	if (ldc_proc_init(wdev->shared_mem) < 0) {
		pr_err("ldc proc init failed\n");
		goto err_proc;
	}
#endif
	atomic_set(&wdev->clk_en, false);
	ldc_clk_init(wdev);
	ldc_dev_init(wdev);

	if (ldc_sw_init(wdev)) {
		pr_err("ldc sw init fail\n");
		goto err_sw_init;
	}

	return rc;

err_sw_init:
	ldc_sw_deinit(wdev);
#ifdef CONFIG_PROC_FS
err_proc:
	if (wdev->shared_mem) {
		kfree(wdev->shared_mem);
		wdev->shared_mem = NULL;
	}
#endif

misc_deregister(&wdev->miscdev);

err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	TRACE_LDC(DBG_ERR, "failed with rc(%d).\n", rc);

	return rc;
}

static int ldc_destroy_instance(struct platform_device *pdev)
{
	struct ldc_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("invalid data\n");
		return -EINVAL;
	}

	ldc_sw_deinit(wdev);
	ldc_dev_deinit(wdev);
	ldc_clk_deinit(wdev);

#ifdef CONFIG_PROC_FS
	ldc_proc_remove();

	if (wdev->shared_mem) {
		kfree(wdev->shared_mem);
		wdev->shared_mem = NULL;
	}
#endif

	misc_deregister(&wdev->miscdev);

	return 0;
}

static int ldc_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct ldc_vdev *wdev;

	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	ldc_dev = wdev;
	dev_set_drvdata(&pdev->dev, wdev);

	rc = ldc_init_resources(pdev);
	if (rc)
		goto err_res;

	rc = ldc_create_instance(pdev);
	if (rc) {
		pr_err("Failed to create ldc instance\n");
		goto err_ldc_creat_ins;
	}

	if (ldc_reg_cb(wdev)) {
		dev_err(&pdev->dev, "Failed to register ldc cb, err %d\n", rc);
		return -EINVAL;
	}

	TRACE_LDC(DBG_INFO, "done with rc(%d).\n", rc);

	return rc;

err_ldc_creat_ins:
err_res:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);

	return rc;
}

static int ldc_remove(struct platform_device *pdev)
{
	if (ldc_destroy_instance(pdev)) {
		dev_err(&pdev->dev, "ldc_destroy_instance fail\n");
	}

	if (ldc_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm ldc cb\n");
	}

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id ldc_dt_match[] = {
	{ .compatible = "cvitek,gdc" },
	{}
};

#ifdef CONFIG_PM_SLEEP
int ldc_suspend(struct device *dev)
{
	int ret = 0;
	struct ldc_vdev * vdev = ldc_get_dev();

	if (unlikely(!vdev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return -1;
	}

	ret = ldc_suspend_handler();
	if (ret) {
		TRACE_LDC(DBG_ERR, "fail to suspend ldc thread, err=%d\n", ret);
		return -1;
	}

	ldc_dev_deinit(vdev);
	ldc_clk_deinit(vdev);
	atomic_set(&suspend_flag, 1);

	TRACE_LDC(DBG_WARN, "ldc suspended\n");
	return 0;
}

int ldc_resume(struct device *dev)
{
	int ret = 0;
	struct ldc_vdev * vdev = ldc_get_dev();

	if (unlikely(!vdev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return -1;
	}

	ldc_clk_init(vdev);
	ldc_dev_init(vdev);

	ret = ldc_resume_handler();
	if (ret) {
		TRACE_LDC(DBG_ERR, "fail to resume ldc thread, err=%d\n", ret);
		return -1;
	}
	atomic_set(&suspend_flag, 0);

	TRACE_LDC(DBG_WARN, "ldc resumed\n");
	return 0;
}

static SIMPLE_DEV_PM_OPS(ldc_pm_ops, ldc_suspend, ldc_resume);
#else
static SIMPLE_DEV_PM_OPS(ldc_pm_ops, NULL, NULL);
#endif

#if (!DEVICE_FROM_DTS)
static void ldc_pdev_release(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
}

static struct platform_device ldc_pdev = {
	.name		= LDC_DEV_NAME,
	.dev.release	= ldc_pdev_release,
};
#endif

static struct platform_driver ldc_driver = {
	.probe      = ldc_probe,
	.remove     = ldc_remove,
	.driver     = {
		.name		= LDC_DEV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= ldc_dt_match,
		.pm		= &ldc_pm_ops,
	},
};

#if 1
static int __init __ldc_init(void)
{
	int rc;

	TRACE_LDC(DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&ldc_driver);
	#else
	rc = platform_device_register(&ldc_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&ldc_driver);
	if (rc)
		platform_device_unregister(&ldc_pdev);
	#endif

	return rc;
}

static void __exit __ldc_exit(void)
{
	TRACE_LDC(DBG_INFO, " +\n");
	platform_driver_unregister(&ldc_driver);
	#if (!DEVICE_FROM_DTS)
		platform_device_unregister(&ldc_pdev);
	#endif
}

module_init(__ldc_init);
module_exit(__ldc_exit);
#else
module_platform_driver(ldc_driver);
#endif

MODULE_DESCRIPTION("Cvitek Video Driver For LDC");
MODULE_AUTHOR("robin.lee");
MODULE_LICENSE("GPL");

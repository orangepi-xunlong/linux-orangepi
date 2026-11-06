#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
//#include <linux/module.h>

#include "dpu_debug.h"
#include "../chip/soph/dpu.h"
#include "dpu_ioctl.h"
#include <linux/dpu_uapi.h>
#include "base_ctx.h"
long dpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct dpu_dev_s *dpu_data;
	struct dpu_handle_info_s *h_info, *h_node;
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

	dpu_data = (struct dpu_dev_s *) filp->private_data;
	if (dpu_data == NULL){
		return -ENOMEM;
	}

	if (dpu_get_handle_info(dpu_data, filp, &h_info)) {
		pr_err("dpudrv: file list is not found!\n");
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size)) {
		TRACE_DPU(DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		TRACE_DPU(DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		TRACE_DPU(DBG_INFO, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case DPU_CREATE_GROUP:
	{
		struct dpu_grp_attr *attr =
			(struct dpu_grp_attr *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_attr);
		ret = dpu_create_grp(attr->dpu_grp_id, &attr->grp_attr);
		break;
	}

	case DPU_DESTROY_GROUP:
	{
		struct dpu_grp_cfg *cfg = ((struct dpu_grp_cfg *)kdata);
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_cfg);
		ret = dpu_destroy_grp(cfg->dpu_grp_id);

		mutex_lock(&dpu_data->dpu_lock);
		list_for_each_entry(h_node, &dpu_data->handle_list, list) {
			if(h_node->open_pid == h_info->open_pid){
				h_info->use_grp[cfg->dpu_grp_id] = FALSE;
			}
		}
		mutex_unlock(&dpu_data->dpu_lock);
		break;
	}

	case DPU_GET_AVAIL_GROUP:
	{
		dpu_grp dpu_grp_id;
		CHECK_IOCTL_CMD(cmd, dpu_grp);
		dpu_grp_id = dpu_get_available_grp();
		*((dpu_grp *)kdata) = dpu_grp_id;
		ret = 0;

		mutex_lock(&dpu_data->dpu_lock);
		list_for_each_entry(h_node, &dpu_data->handle_list, list) {
			if(h_node->open_pid == h_info->open_pid){
				h_info->use_grp[dpu_grp_id] = TRUE;
			}
		}
		mutex_unlock(&dpu_data->dpu_lock);
		break;
	}

	case DPU_START_GROUP:
	{
		struct dpu_grp_cfg *cfg = (struct dpu_grp_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_cfg);
		ret = dpu_start_grp(cfg->dpu_grp_id);
		break;
	}

	case DPU_STOP_GROUP:
	{
		struct dpu_grp_cfg *cfg = (struct dpu_grp_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_cfg);
		ret = dpu_stop_grp(cfg->dpu_grp_id);
		break;
	}

	case DPU_SET_GRP_ATTR:
	{
		dpu_grp dpu_grp_id;
		const dpu_grp_attr_s *grp_attr;
		struct dpu_grp_attr *cfg = (struct dpu_grp_attr *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_attr);
		dpu_grp_id = cfg->dpu_grp_id;

		grp_attr = &cfg->grp_attr;

		ret = dpu_set_grp_attr(dpu_grp_id, grp_attr);
		break;
	}

	case DPU_GET_GRP_ATTR:
	{
		dpu_grp dpu_grp_id;
		dpu_grp_attr_s *grp_attr;
		struct dpu_grp_attr *cfg = (struct dpu_grp_attr *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_grp_attr);
		dpu_grp_id = cfg->dpu_grp_id;
		grp_attr = &cfg->grp_attr;

		ret = dpu_get_grp_attr(dpu_grp_id, grp_attr);
		break;
	}

	case DPU_SEND_FRAME:
	{
		dpu_grp dpu_grp_id;
		const video_frame_info_s *src_left_frame;
		const video_frame_info_s *src_right_frame;
		int millisec;
		struct dpu_set_frame_cfg *cfg = (struct dpu_set_frame_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_set_frame_cfg);
		dpu_grp_id = cfg->dpu_grp_id;
		src_left_frame = &cfg->src_left_frame;
		src_right_frame = &cfg->src_right_frame;
		millisec = cfg->millisec;

		ret = dpu_send_frame(dpu_grp_id, src_left_frame,\
								 src_right_frame,millisec);
		break;
	}

	case DPU_SEND_CHN_FRAME:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		video_frame_info_s *video_frame;
		int millisec;
		struct dpu_get_frame_cfg *cfg = (struct dpu_get_frame_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_get_frame_cfg);
		dpu_grp_id = cfg->dpu_grp_id;
		dpu_chn_id = cfg->dpu_chn_id;
		video_frame = &cfg->video_frame;

		millisec = cfg->millisec;
		ret =  dpu_send_chn_frame(dpu_grp_id, dpu_chn_id, video_frame, millisec);

		break;
	}

	case DPU_GET_FRAME:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		video_frame_info_s *video_frame;
		int millisec;
		struct dpu_get_frame_cfg *cfg = (struct dpu_get_frame_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_get_frame_cfg);
		dpu_grp_id = cfg->dpu_grp_id;
		dpu_chn_id = cfg->dpu_chn_id;
		video_frame = &cfg->video_frame;

		millisec = cfg->millisec;
		ret =  dpu_get_frame(dpu_grp_id,dpu_chn_id,video_frame, millisec);

		break;
	}

	case DPU_RELEASE_FRAME:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		video_frame_info_s *video_frame;
		struct dpu_release_frame_cfg *cfg = (struct dpu_release_frame_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_release_frame_cfg);
		dpu_grp_id = cfg->dpu_grp_id;
		dpu_chn_id = cfg->dpu_chn_id;
		video_frame = &cfg->video_frame;
		ret =  dpu_release_frame(dpu_grp_id,dpu_chn_id, video_frame);
		break;
	}

	case DPU_SET_CHN_ATTR:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		const dpu_chn_attr_s *chn_attr;
		struct dpu_chn_attr *attr = (struct dpu_chn_attr *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_chn_attr);
		dpu_grp_id = attr->dpu_grp_id;
		dpu_chn_id = attr->dpu_chn_id;

		chn_attr = &attr->chn_attr;

		ret = dpu_set_chn_attr(dpu_grp_id, dpu_chn_id, chn_attr);
		break;
	}

	case DPU_GET_CHN_ATTR:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		dpu_chn_attr_s *chn_attr;
		struct dpu_chn_attr *attr = (struct dpu_chn_attr *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_chn_attr);
		dpu_grp_id = attr->dpu_grp_id;
		dpu_chn_id = attr->dpu_chn_id;
		chn_attr = &attr->chn_attr;

		ret = dpu_get_chn_attr(dpu_grp_id, dpu_chn_id, chn_attr);
		break;
	}

	case DPU_ENABLE_CHN:
	{
		dpu_grp dpu_grp_id;
		dpu_chn dpu_chn_id;
		struct dpu_chn_cfg *cfg = (struct dpu_chn_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_chn_cfg);
		dpu_grp_id = cfg->dpu_grp_id;
		dpu_chn_id = cfg->dpu_chn_id;

		ret = dpu_enable_chn(dpu_grp_id, dpu_chn_id);
		break;
	}

	case DPU_DISABLE_CHN:
	{
		struct dpu_chn_cfg *cfg = (struct dpu_chn_cfg *)kdata;
		CHECK_IOCTL_CMD(cmd, struct dpu_chn_cfg);
		ret = dpu_disable_chn(cfg->dpu_grp_id, cfg->dpu_chn_id);
		break;
	}

	case DPU_CHECK_REG_READ:
	{
		dpu_check_reg_read();
		break;
	}

	case DPU_CHECK_REG_WRITE:
	{
		dpu_check_reg_write();
		break;
	}

	case DPU_GET_SGBM_STATUS:
	{
		getsgbm_status();
		break;
	}

	case DPU_GET_FGS_STATUS:
	{
		getfgs_status();
		break;
	}


	default:
		TRACE_DPU(DBG_DEBUG, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}

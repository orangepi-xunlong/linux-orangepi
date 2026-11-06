#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "base_ctx.h"

#include "vpss_debug.h"
#include "vpss_proc.h"
#include "vpss_common.h"
#include "scaler.h"
#include "vpss_core.h"
#include "vpss.h"

#define VPSS_PROC_NAME          "soph/vpss"
#define VPP_PROC_NAME          "soph/vppinfo"

// for proc info
static int proc_vpss_mode;
static const char * const vb_source[] = {"CommonVB", "UserVB", "UserIon"};
static const char * const vpss_name[] = {"vpss_v0", "vpss_v1", "vpss_v2", "vpss_v3",
										"vpss_t0", "vpss_t1", "vpss_t2", "vpss_t3",
										"vpss_d0", "vpss_d1"};

/*************************************************************************
 *	VPSS proc functions
 *************************************************************************/
static void _pix_fmt_to_string(pixel_format_e pix_fmt, char *str, int len)
{
	switch (pix_fmt) {
	case PIXEL_FORMAT_RGB_888:
		strncpy(str, "RGB_888", len);
		break;
	case PIXEL_FORMAT_BGR_888:
		strncpy(str, "BGR_888", len);
		break;
	case PIXEL_FORMAT_RGB_888_PLANAR:
		strncpy(str, "RGB_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_BGR_888_PLANAR:
		strncpy(str, "BGR_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_8BPP:
		strncpy(str, "RGB_BAYER_8BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_10BPP:
		strncpy(str, "RGB_BAYER_10BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_12BPP:
		strncpy(str, "RGB_BAYER_12BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_14BPP:
		strncpy(str, "RGB_BAYER_14BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_16BPP:
		strncpy(str, "RGB_BAYER_16BPP", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strncpy(str, "YUV_PLANAR_422", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strncpy(str, "YUV_PLANAR_420", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strncpy(str, "YUV_PLANAR_444", len);
		break;
	case PIXEL_FORMAT_YUV_400:
		strncpy(str, "YUV_400", len);
		break;
	case PIXEL_FORMAT_HSV_888:
		strncpy(str, "HSV_888", len);
		break;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		strncpy(str, "HSV_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_NV12:
		strncpy(str, "NV12", len);
		break;
	case PIXEL_FORMAT_NV21:
		strncpy(str, "NV21", len);
		break;
	case PIXEL_FORMAT_NV16:
		strncpy(str, "NV16", len);
		break;
	case PIXEL_FORMAT_NV61:
		strncpy(str, "NV61", len);
		break;
	case PIXEL_FORMAT_YUYV:
		strncpy(str, "YUYV", len);
		break;
	case PIXEL_FORMAT_UYVY:
		strncpy(str, "UYVY", len);
		break;
	case PIXEL_FORMAT_YVYU:
		strncpy(str, "YVYU", len);
		break;
	case PIXEL_FORMAT_VYUY:
		strncpy(str, "VYUY", len);
		break;
	case PIXEL_FORMAT_YUV_444:
		strncpy(str, "YUV_444", len);
		break;
	case PIXEL_FORMAT_FP32_C1:
		strncpy(str, "FP32_C1", len);
		break;
	case PIXEL_FORMAT_FP32_C3_PLANAR:
		strncpy(str, "FP32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT32_C1:
		strncpy(str, "INT32_C1", len);
		break;
	case PIXEL_FORMAT_INT32_C3_PLANAR:
		strncpy(str, "INT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT32_C1:
		strncpy(str, "UINT32_C1", len);
		break;
	case PIXEL_FORMAT_UINT32_C3_PLANAR:
		strncpy(str, "UINT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_FP16_C1:
		strncpy(str, "FP16_C1", len);
		break;
	case PIXEL_FORMAT_FP16_C3_PLANAR:
		strncpy(str, "FP16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_BF16_C1:
		strncpy(str, "BF16_C1", len);
		break;
	case PIXEL_FORMAT_BF16_C3_PLANAR:
		strncpy(str, "BF16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT16_C1:
		strncpy(str, "INT16_C1", len);
		break;
	case PIXEL_FORMAT_INT16_C3_PLANAR:
		strncpy(str, "INT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT16_C1:
		strncpy(str, "UINT16_C1", len);
		break;
	case PIXEL_FORMAT_UINT16_C3_PLANAR:
		strncpy(str, "UINT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT8_C1:
		strncpy(str, "INT8_C1", len);
		break;
	case PIXEL_FORMAT_INT8_C3_PLANAR:
		strncpy(str, "INT8_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT8_C1:
		strncpy(str, "UINT8_C1", len);
		break;
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		strncpy(str, "UINT8_C3_PLANAR", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

int vpss_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char c[32];
	vpss_mod_param_s mod_param;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();
	struct vpss_device *dev = (struct vpss_device *)m->private;
	signed int proc_amp[PROC_AMP_MAX];

	vpss_get_mod_param(&mod_param);

	// Module Param
	seq_printf(m, "\nModule: [VPSS], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------\n");
	seq_printf(m, "%25s\n", "vpss_vb_source");
	seq_printf(m, "%25s\n", vb_source[mod_param.vpss_buf_source]);

	// VPSS GRP ATTR
	seq_puts(m, "\n-------------------------------VPSS GRP ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s\n", "GrpID", "MaxW", "MaxH", "pix_fmt",
				"SrcFRate", "DstFRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(vpss_ctx[i]->grp_attr.pixel_format, c, sizeof(c));

			seq_printf(m, "%8s%2d%10d%10d%20s%10d%10d\n",
				"#",
				i,
				vpss_ctx[i]->grp_attr.w,
				vpss_ctx[i]->grp_attr.h,
				c,
				vpss_ctx[i]->grp_attr.frame_rate.src_frame_rate,
				vpss_ctx[i]->grp_attr.frame_rate.dst_frame_rate);
		}
	}

	// VPSS GRP AMP CTRL
	seq_puts(m, "\n-------------------------------VPSS GRP AMP CTRL------------------------------\n");
	seq_printf(m, "%10s%15s%13s%15s%8s\n", "GrpID", "Brightness", "Contrast", "Saturation",
				"Hue");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			vpss_get_proc_amp(i, proc_amp);

			seq_printf(m, "%8s%2d%15d%13d%15d%8d\n",
				"#",
				i,
				proc_amp[PROC_AMP_BRIGHTNESS],
				proc_amp[PROC_AMP_CONTRAST],
				proc_amp[PROC_AMP_SATURATION],
				proc_amp[PROC_AMP_HUE]);

		}
	}

	//VPSS CHN ATTR
	seq_puts(m, "\n-------------------------------VPSS CHN ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "PhyChnID", "Enable", "MirrorEn", "FlipEn", "SrcFRate", "DstFRate");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"Depth", "Aspect", "videoX", "videoY", "videoW", "videoH", "BgColor");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				int32_t x, y;
				uint32_t w, h;

				memset(c, 0, sizeof(c));
				if (vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.mode == ASPECT_RATIO_NONE)
					strncpy(c, "NONE", sizeof(c));
				else if (vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.mode == ASPECT_RATIO_AUTO)
					strncpy(c, "AUTO", sizeof(c));
				else if (vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.mode
						== ASPECT_RATIO_MANUAL)
					strncpy(c, "MANUAL", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				if (vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
					x = vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.video_rect.x;
					y = vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.video_rect.y;
					w = vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.video_rect.width;
					h = vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.video_rect.height;
				} else {
					x = y = 0;
					w = h = 0;
				}

				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d\n%10d%10s%10d%10d%10d%10d%#10x\n",
					"#",
					i,
					"#",
					j,
					(vpss_ctx[i]->chn_cfgs[j].is_enabled) ? "y" : "N",
					(vpss_ctx[i]->chn_cfgs[j].chn_attr.mirror) ? "y" : "N",
					(vpss_ctx[i]->chn_cfgs[j].chn_attr.flip) ? "y" : "N",
					vpss_ctx[i]->chn_cfgs[j].chn_attr.frame_rate.src_frame_rate,
					vpss_ctx[i]->chn_cfgs[j].chn_attr.frame_rate.dst_frame_rate,
					vpss_ctx[i]->chn_cfgs[j].chn_attr.depth,
					c,
					x,
					y,
					w,
					h,
					vpss_ctx[i]->chn_cfgs[j].chn_attr.aspect_ratio.bgcolor);
			}
		}
	}

	// VPSS GRP CROP INFO
	seq_puts(m, "\n-------------------------------VPSS GRP CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%10d\n",
				"#",
				i,
				(vpss_ctx[i]->grp_crop_info.enable) ? "y" : "N",
				(vpss_ctx[i]->grp_crop_info.crop_coordinate == VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
				vpss_ctx[i]->grp_crop_info.crop_rect.x,
				vpss_ctx[i]->grp_crop_info.crop_rect.y,
				vpss_ctx[i]->grp_crop_info.crop_rect.width,
				vpss_ctx[i]->grp_crop_info.crop_rect.height);
		}
	}

	// VPSS CHN CROP INFO
	seq_puts(m, "\n-------------------------------VPSS CHN CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(vpss_ctx[i]->chn_cfgs[j].crop_info.enable) ? "y" : "N",
					(vpss_ctx[i]->chn_cfgs[j].crop_info.crop_coordinate
						== VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
					vpss_ctx[i]->chn_cfgs[j].crop_info.crop_rect.x,
					vpss_ctx[i]->chn_cfgs[j].crop_info.crop_rect.y,
					vpss_ctx[i]->chn_cfgs[j].crop_info.crop_rect.width,
					vpss_ctx[i]->chn_cfgs[j].crop_info.crop_rect.height);
			}
		}
	}

	// VPSS GRP WORK STATUS
	seq_puts(m, "\n-------------------------------VPSS GRP WORK STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%20s%20s%20s%20s\n",
		"GrpID", "RecvCnt", "LostCnt", "StartFailCnt", "bStart",
		"CostTime(us)", "MaxCostTime(us)",
		"HwCostTime(us)", "HwMaxCostTime(us)");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			seq_printf(m, "%8s%2d%10d%10d%20d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				vpss_ctx[i]->grp_work_status.recv_cnt,
				vpss_ctx[i]->grp_work_status.lost_cnt,
				vpss_ctx[i]->grp_work_status.start_fail_cnt,
				(vpss_ctx[i]->is_started) ? "y" : "N",
				vpss_ctx[i]->grp_work_status.cost_time,
				vpss_ctx[i]->grp_work_status.max_cost_time,
				vpss_ctx[i]->grp_work_status.hw_cost_time,
				vpss_ctx[i]->grp_work_status.hw_max_cost_time);
		}
	}

	// VPSS CHN OUTPUT RESOLUTION
	seq_puts(m, "\n-------------------------------VPSS CHN OUTPUT RESOLUTION-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "Enable", "Width", "Height", "Pixfmt", "Videofmt", "VbPool", "SendOK", "FrameRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				_pix_fmt_to_string(vpss_ctx[i]->chn_cfgs[j].chn_attr.pixel_format, c, sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%20s%10s%9d%1s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(vpss_ctx[i]->chn_cfgs[j].is_enabled) ? "y" : "N",
					vpss_ctx[i]->chn_cfgs[j].chn_attr.width,
					vpss_ctx[i]->chn_cfgs[j].chn_attr.height,
					c,
					(vpss_ctx[i]->chn_cfgs[j].chn_attr.video_format
						== VIDEO_FORMAT_LINEAR) ? "LINEAR" : "UNKNOWN",
					vpss_ctx[i]->chn_cfgs[j].chn_work_status.pool_id,
					(vpss_ctx[i]->chn_cfgs[j].vb_pool == VB_INVALID_POOLID) ? " ": "*",
					vpss_ctx[i]->chn_cfgs[j].chn_work_status.send_ok,
					vpss_ctx[i]->chn_cfgs[j].chn_work_status.real_frame_rate);
			}
		}
	}

	// VPSS CHN ROTATE INFO
	seq_puts(m, "\n-------------------------------VPSS CHN ROTATE INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "GrpID", "ChnID", "Rotate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				if (vpss_ctx[i]->chn_cfgs[j].rotation == ROTATION_0)
					strncpy(c, "0", sizeof(c));
				else if (vpss_ctx[i]->chn_cfgs[j].rotation == ROTATION_90)
					strncpy(c, "90", sizeof(c));
				else if (vpss_ctx[i]->chn_cfgs[j].rotation == ROTATION_180)
					strncpy(c, "180", sizeof(c));
				else if (vpss_ctx[i]->chn_cfgs[j].rotation == ROTATION_270)
					strncpy(c, "270", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s\n", "#", i, "#", j, c);
			}
		}
	}

	// VPSS CHN LDC INFO
	seq_puts(m, "\n-------------------------------VPSS CHN LDC INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n", "GrpID", "ChnID", "Enable", "Aspect", "XRatio", "YRatio");
	seq_printf(m, "%10s%10s%10s%20s\n", "XYRatio", "XOffset", "YOffset", "DistortionRatio");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpss_ctx[i] && vpss_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d\n%10d%10d%10d%20d\n",
					"#",
					i,
					"#",
					j,
					(vpss_ctx[i]->chn_cfgs[j].ldc_attr.enable) ? "y" : "N",
					(vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.aspect) ? "y" : "N",
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.x_ratio,
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.y_ratio,
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.xy_ratio,
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.center_x_offset,
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.center_y_offset,
					vpss_ctx[i]->chn_cfgs[j].ldc_attr.attr.distortion_ratio);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VPSS HW STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"ID", "Dev", "Online", "Status", "StartCnt", "IntCnt", "DutyRatio");
	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		int state = atomic_read(&dev->vpss_cores[i].state);

		memset(c, 0, sizeof(c));
		if (state == VIP_IDLE)
			strncpy(c, "Idle", sizeof(c));
		else if (state == VIP_RUNNING)
			strncpy(c, "Running", sizeof(c));
		else if (state == VIP_END)
			strncpy(c, "End", sizeof(c));
		else if (state == VIP_ONLINE)
			strncpy(c, "Online", sizeof(c));

		seq_printf(m, "%8s%2d%10s%10s%10s%10d%10d%10d\n",
			"#",
			i,
			vpss_name[i],
			dev->vpss_cores[i].is_online ? "y" : "N",
			c,
			dev->vpss_cores[i].start_cnt,
			dev->vpss_cores[i].int_cnt,
			dev->vpss_cores[i].duty_ratio);
	}

	return 0;
}

int vpp_ctx_proc_show(struct seq_file *m, void *v)
{
	int i;
	struct vpss_device *dev = (struct vpss_device *)m->private;

	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		seq_printf(m, "{\"id\":%d, \"usage(instant|long)\":%10d%%|%10d%% \n", i, dev->vpss_cores[i].duty_ratio,\
                dev->vpss_cores[i].duty_ratio_long);
	}

	return 0;
}

static int vpss_proc_show(struct seq_file *m, void *v)
{
	// show driver status if vpss_mode == 1
	//if (proc_vpss_mode) {
	//}
	return vpss_ctx_proc_show(m, v);
}

static int vpp_proc_show(struct seq_file *m, void *v)
{

	return vpp_ctx_proc_show(m, v);
}

static ssize_t vpss_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char proc_input_data[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(proc_input_data)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(proc_input_data, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(proc_input_data, 10, &proc_vpss_mode))
		proc_vpss_mode = 0;

	return count;
}

static int vpss_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpss_proc_show, PDE_DATA(inode));
}

static ssize_t vpp_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char proc_input_data[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(proc_input_data)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(proc_input_data, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	return count;
}

static int vpp_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpp_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vpss_proc_fops = {
	.proc_open = vpss_proc_open,
	.proc_read = seq_read,
	.proc_write = vpss_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vpss_proc_fops = {
	.owner = THIS_MODULE,
	.open = vpss_proc_open,
	.read = seq_read,
	.write = vpss_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vpp_proc_fops = {
	.proc_open = vpp_proc_open,
	.proc_read = seq_read,
	.proc_write = vpp_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vpp_proc_fops = {
	.owner = THIS_MODULE,
	.open = vpp_proc_open,
	.read = seq_read,
	.write = vpp_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vpss_proc_init(struct vpss_device *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(VPSS_PROC_NAME, 0644, NULL,
				 &vpss_proc_fops, dev);
	if (!entry) {
		TRACE_VPSS(DBG_ERR, "vpss proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int vpss_proc_remove(struct vpss_device *dev)
{
	remove_proc_entry(VPSS_PROC_NAME, NULL);
	return 0;
}

int vpp_proc_init(struct vpss_device *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(VPP_PROC_NAME, 0644, NULL,
				 &vpp_proc_fops, dev);
	if (!entry) {
		TRACE_VPSS(DBG_ERR, "vpss proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int vpp_proc_remove(struct vpss_device *dev)
{
	remove_proc_entry(VPP_PROC_NAME, NULL);
	return 0;
}
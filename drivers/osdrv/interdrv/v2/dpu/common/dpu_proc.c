#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "dpu_debug.h"
#include <linux/comm_dpu.h>
#include "../chip/soph/dpu.h"
#include <linux/common.h>
#include <linux/defines.h>
#include "dpu_proc.h"


#define DPU_SHARE_MEM_SIZE     (0x8000)
#define DPU_PROC_NAME          "soph/dpu"

// for proc info
static int proc_dpu_mode;

/*************************************************************************
 *	DPU proc functions
 *************************************************************************/
static void _pix_fmt_to_string(enum _pixel_format_e pixfmt, char *str, int len)
{
	switch (pixfmt) {
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

static void _mask_mode_to_string(dpu_mask_mode_e mask_mode, char *str, int len)
{
	switch (mask_mode) {
	case DPU_MASK_MODE_DEFAULT:
		strncpy(str, "7x7", len);
		break;
	case DPU_MASK_MODE_1x1:
		strncpy(str, "1x1", len);
		break;
	case DPU_MASK_MODE_3x3:
		strncpy(str, "3x3", len);
		break;
	case DPU_MASK_MODE_5x5:
		strncpy(str, "5x5", len);
		break;
	case DPU_MASK_MODE_7x7:
		strncpy(str, "7x7", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static void _dcc_dir_to_string(dpu_dcc_dir_e dcc_dir, char *str, int len)
{
	switch (dcc_dir) {
	case DPU_DCC_DIR_DEFAULT:
		strncpy(str, "A12", len);
		break;
	case DPU_DCC_DIR_A12:
		strncpy(str, "A12", len);
		break;
	case DPU_DCC_DIR_A13:
		strncpy(str, "A13", len);
		break;
	case DPU_DCC_DIR_A14:
		strncpy(str, "A14", len);
		break;
	default:
		strncpy(str, "Unknown DCC_DIR Fmt", len);
		break;
	}
}

static void _depth_unit_to_string(dpu_depth_unit_e dpu_depth_unit, char *str, int len)
{
	switch (dpu_depth_unit) {
	case DPU_DEPTH_UNIT_DEFAULT:
		strncpy(str, "MM", len);
		break;
	case DPU_DEPTH_UNIT_MM:
		strncpy(str, "MM", len);
		break;
	case DPU_DEPTH_UNIT_CM:
		strncpy(str, "CM", len);
		break;
	case DPU_DEPTH_UNIT_DM:
		strncpy(str, "DM", len);
		break;
	case DPU_DEPTH_UNIT_M:
		strncpy(str, "M", len);
		break;
	default:
		strncpy(str, "Unknown DEPTH_UNIT Fmt", len);
		break;
	}
}

static void _dpu_mode_to_string(dpu_mode_e dpu_mode, char *str, int len)
{
	switch (dpu_mode) {
	case DPU_MODE_DEFAULT:
		strncpy(str, "SGBM_MUX0", len);
		break;
	case DPU_MODE_SGBM_MUX0:
		strncpy(str, "SGBM_MUX0", len);
		break;
	case DPU_MODE_SGBM_MUX1:
		strncpy(str, "SGBM_MUX1", len);
		break;
	case DPU_MODE_SGBM_MUX2:
		strncpy(str, "SGBM_MUX2", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX0:
		strncpy(str, "ONLINE_MUX0", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX1:
		strncpy(str, "ONLINE_MUX1", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX2:
		strncpy(str, "ONLINE_MUX1", len);
		break;
	case DPU_MODE_FGS_MUX0:
		strncpy(str, "FGS_MUX0", len);
		break;
	case DPU_MODE_FGS_MUX1:
		strncpy(str, "FGS_MUX1", len);
		break;
	default:
		strncpy(str, "Unknown dpuMode Fmt", len);
		break;
	}
}

static void _disp_range_to_string(dpu_disp_range_e disp_range, char *str, int len)
{
	switch (disp_range) {
	case DPU_DISP_RANGE_DEFAULT:
		strncpy(str, "16", len);
		break;
	case DPU_DISP_RANGE_16:
		strncpy(str, "16", len);
		break;
	case DPU_DISP_RANGE_32:
		strncpy(str, "32", len);
		break;
	case DPU_DISP_RANGE_48:
		strncpy(str, "48", len);
		break;
	case DPU_DISP_RANGE_64:
		strncpy(str, "64", len);
		break;
	case DPU_DISP_RANGE_80:
		strncpy(str, "80", len);
		break;
	case DPU_DISP_RANGE_96:
		strncpy(str, "96", len);
		break;
	case DPU_DISP_RANGE_112:
		strncpy(str, "112", len);
		break;
	case DPU_DISP_RANGE_128:
		strncpy(str, "128", len);
		break;
	default:
		strncpy(str, "Unknown DISP RANGE", len);
		break;
	}
}

int dpu_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char c[50];
	char cmaskmode[50];
	char cdpumode[50];
	char cdccdir[50];
	char cdisprange[50];
	char cdepthunit[50];
	char is_started[10] ;
	char is_created[10] ;
	//struct dpu_dev_s *bdev = m->private;
	struct dpu_dev_s *dev = dpu_get_dev();
	struct dpu_ctx_s **p_dpu_ctx = dpu_get_shdw_ctx();

	// Module Param
	seq_printf(m, "\nModule: [DPU], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------DPU HardWare STATUS-------------------------------\n");
	seq_printf(m, "%20s%20s\n", "Busy", "STCnt");
	//
	// DPU GRP ATTR
	seq_puts(m, "\n-------------------------------DPU GRP ATTR1------------------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s%20s%20s%20s\n", "grp_id", "bStart", "bCreate", \
				"DevID","SrcFRate", "DstFRate", "LeftWidth","LeftHeight","RightWidth","RightHeight");

	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
			// seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s%20s%20s%20s\n", "grp_id", "bStart", "bCreate",
			// 	"DevID","SrcFRate", "DstFRate", "LeftWidth","LeftHeight","RightWidth","RightHeight");
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(p_dpu_ctx[i]->pixel_format, c, sizeof(c));

			if(p_dpu_ctx[i]->isstarted)
				strncpy(is_started, "Y", sizeof(is_started));
			else
				strncpy(is_started, "N", sizeof(is_started));;

			if(p_dpu_ctx[i]->iscreated)
				strncpy(is_created, "Y", sizeof(is_created));
			else
				strncpy(is_created, "N", sizeof(is_created));

			seq_printf(m, "%20d%20s%20s%20d%20d%20d%20d%20d%20d%20d\n",
				i,
				is_started,
				is_created,
				p_dpu_ctx[i]->dpu_dev_id,
				p_dpu_ctx[i]->grp_attr.frame_rate.src_frame_rate,
				p_dpu_ctx[i]->grp_attr.frame_rate.dst_frame_rate,
				p_dpu_ctx[i]->grp_attr.left_image_size.width,
				p_dpu_ctx[i]->grp_attr.left_image_size.height,
				p_dpu_ctx[i]->grp_attr.right_image_size.width,
				p_dpu_ctx[i]->grp_attr.right_image_size.height
				);
		}
	}

	// DPU GRP ATTR
	seq_puts(m, "\n-------------------------------DPU GRP ATTR2------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%15s%20s%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s%15s\n", "grp_id",
				"MaskMode", "DpuMode","DispRange","DccDir  ","DepthUnit","DispStartPos","Rshift1","Rshift2",
				"CaP1","CaP2","UniqRatio","DispShift","CensusShift","FxBaseline","FgsMaxCount","FgsMaxT","isbtcostout");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
			// seq_printf(m, "%10s%10s%10s%10s%10s%15s%10s%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s%15s\n", "grp_id",
			// 	"MaskMode", "DpuMode","DispRange","DccDir  ","DepthUnit","DispStartPos","Rshift1","Rshift2",
			// 	"CaP1","CaP2","UniqRatio","DispShift","CensusShift","FxBaseline","FgsMaxCount","FgsMaxT","isbtcostout");
			memset(cmaskmode, 0, sizeof(cmaskmode));
			memset(cdpumode, 0, sizeof(cdpumode));
			memset(cdccdir, 0, sizeof(cdccdir));
			memset(cdisprange, 0, sizeof(cdisprange));
			memset(cdepthunit, 0, sizeof(cdepthunit));

			_mask_mode_to_string(p_dpu_ctx[i]->grp_attr.mask_mode, cmaskmode, sizeof(cmaskmode));
			_dpu_mode_to_string(p_dpu_ctx[i]->grp_attr.dpu_mode, cdpumode, sizeof(cdpumode));
			_dcc_dir_to_string(p_dpu_ctx[i]->grp_attr.dcc_dir,cdccdir,sizeof(cdccdir));
			_depth_unit_to_string(p_dpu_ctx[i]->grp_attr.dpu_depth_unit,cdepthunit,sizeof(cdepthunit));
			_disp_range_to_string(p_dpu_ctx[i]->grp_attr.disp_range,cdisprange,sizeof(cdisprange));
			seq_printf(m, "%10d%10s%10s%10s%10s%15s%20d%10d%10d%10d%10d%10d%15d%15d%15d%15d%15d%15d\n",
				i,
				cmaskmode,
				cdpumode,
				cdisprange,
				cdccdir,
				cdepthunit,
				p_dpu_ctx[i]->grp_attr.dispstartpos,
				p_dpu_ctx[i]->grp_attr.rshift1,
				p_dpu_ctx[i]->grp_attr.rshift2,
				p_dpu_ctx[i]->grp_attr.cap1,
				p_dpu_ctx[i]->grp_attr.cap2,
				p_dpu_ctx[i]->grp_attr.uniqratio,
				p_dpu_ctx[i]->grp_attr.dispshift,
				p_dpu_ctx[i]->grp_attr.censusshift,
				p_dpu_ctx[i]->grp_attr.fxbaseline,
				p_dpu_ctx[i]->grp_attr.fgsmaxcount,
				p_dpu_ctx[i]->grp_attr.fgsmaxt,
				p_dpu_ctx[i]->grp_attr.isbtcostout);
		}
	}

	//DPU CHN ATTR
	seq_puts(m, "\n-------------------------------DPU CHN ATTR------------------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s\n",
							"grp_id", "ChnID", "Enable", "Width", "Height");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
			for (j = 0; j < p_dpu_ctx[i]->chn_num; ++j) {
				char *is_enabled ="Y";
				// seq_printf(m, "%20s%20s%20s%20s%20s\n",
				// 			"grp_id", "ChnID", "Enable", "Width", "Height");
				if(p_dpu_ctx[i]->chn_cfgs[i].isenabled)
					is_enabled = "Y";
				else
					is_enabled = "N";

				seq_printf(m, "%20d%20d%20s%20d%20d\n",
					i,
					j,
					is_enabled,
					p_dpu_ctx[i]->chn_cfgs[j].chn_attr.img_size.width,
					p_dpu_ctx[i]->chn_cfgs[j].chn_attr.img_size.height);
			}
		}
	}

	//DPU INPUT JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU INPUT JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"grp_id", "busy_num", "free_num");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"grp_id", "busy_num", "free_num");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				p_dpu_ctx[i]->input_job_status.busy_num,
	// 				p_dpu_ctx[i]->input_job_status.free_num);
	// 	}
	// }

	//DPU WORKING JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU WORKING JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"grp_id", "busy_num", "free_num");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"grp_id", "busy_num", "free_num");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				p_dpu_ctx[i]->working_job_status.busy_num,
	// 				p_dpu_ctx[i]->working_job_status.free_num);
	// 	}
	// }

	//DPU OUTPUT JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU OUTPUT JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"grp_id", "busy_num", "free_num");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"grp_id", "busy_num", "free_num");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				p_dpu_ctx[i]->output_job_status.busy_num,
	// 				p_dpu_ctx[i]->output_job_status.free_num);
	// 	}
	// }

	// DPU GRP WORK STATUS
	seq_puts(m, "\n-------------------------------DPU GRP WORK STATUS-----------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s\n",
						"grp_id", "FrameNumPerSec", "start_cnt", "FailCnt", "DoneCnt",
						"cur_task_cost_tm(us)","max_task_cost_tm(us),d");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (p_dpu_ctx[i] && p_dpu_ctx[i]->iscreated) {
			// seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s\n",
			// 			"grp_id", "FrameNumPerSec", "start_cnt", "FailCnt", "DoneCnt",
			// 			"cur_task_cost_tm(us)","max_task_cost_tm(us)");
			seq_printf(m, "%20d%20d%20d%20d%20d%20d%20d\n",
				i,
				p_dpu_ctx[i]->grp_work_wtatus.frame_rate,
				p_dpu_ctx[i]->grp_work_wtatus.start_cnt,
				p_dpu_ctx[i]->grp_work_wtatus.start_fail_cnt,
				p_dpu_ctx[i]->grp_work_wtatus.send_pic_cnt,
				p_dpu_ctx[i]->grp_work_wtatus.cur_task_cost_tm,
				p_dpu_ctx[i]->grp_work_wtatus.max_task_cost_tm);
		}
	}

	// DPU Run Time STATUS
	seq_puts(m, "\n-------------------------------DPU RUN TIME STATUS-----------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s\n",
		"DevID", "cnt_per_sec","max_cnt_per_sec","total_int_cnt", "Hwcost_tm(us)", "HwMaxcost_tm(us)","duty_ratio(%)");
			seq_printf(m, "%20d%20d%20d%20d%20d%20d%20d\n",
				0,
				dev->run_time_info.cnt_per_sec,
				dev->run_time_info.max_cnt_per_sec,
				dev->run_time_info.total_int_cnt,
				dev->run_time_info.cost_tm,
				dev->run_time_info.max_cost_tm,
				dev->duty_ratio
			);
	return 0;
}

/*************************************************************************
 *	Proc functions
 *************************************************************************/

static int dpu_proc_show(struct seq_file *m, void *v)
{
	return dpu_ctx_proc_show(m, v);
}

static ssize_t dpu_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_dpu_mode))
		proc_dpu_mode = 0;

	return count;
}

static int dpu_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpu_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops dpu_proc_fops = {
	.proc_open = dpu_proc_open,
	.proc_read = seq_read,
	.proc_write = dpu_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations dpu_proc_fops = {
	.owner = THIS_MODULE,
	.open = dpu_proc_open,
	.read = seq_read,
	.write = dpu_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


int dpu_proc_init(struct dpu_dev_s *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(DPU_PROC_NAME, 0644, NULL,
				 &dpu_proc_fops, dev);
	if (!entry) {
		TRACE_DPU(DBG_ERR, "dpu proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int dpu_proc_remove(struct dpu_dev_s *dev)
{
	remove_proc_entry(DPU_PROC_NAME, NULL);
	return 0;
}

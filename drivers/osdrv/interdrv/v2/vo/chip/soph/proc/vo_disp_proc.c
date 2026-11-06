#include <proc/vo_disp_proc.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "disp.h"

#define VO_DISP_PRC_NAME "soph/vo_disp"
static const char * const str_disp_fmt[] = {"YUV420P", "YUV422P", "RGB888_PLANAR", "RGB888_PACKED", "BGR888_PACKED"
	, "Y_ONLY", "BF16", "", "NV12", "NV21", "NV16", "NV61", "YVYU", "YUYV", "VYUY", "UYVY"};
static const char * const str_disp_csc[] = {"Disable", "2RGB_601_Limit", "2RGB_601_Full", "2RGB_709_Limit"
	, "2RGB_709_Full", "2YUV_601_Limit", "2YUV_601_Full", "2YUV_709_Limit", "2YUV_709_Full"};

/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void show_mem(struct seq_file *m, struct disp_mem *mem)
{
	seq_printf(m, "start_x(%3d)\t\tstart_y(%3d)\t\twidth(%4d)\t\theight(%4d)\n"
		  , mem->start_x, mem->start_y, mem->width, mem->height);
	seq_printf(m, "pitch_y(%3d)\t\tpitch_c(%3d)\n", mem->pitch_y, mem->pitch_c);
}

static void vo_disp_show_disp_status(struct seq_file *m)
{
	int i;

	for (i = 0 ; i < VO_MAX_DEV_NUM; ++i) {
		struct disp_cfg *cfg = disp_get_cfg(i);
		union disp_dbg_status status = disp_get_dbg_status(i, true);
		struct disp_timing *timing = disp_get_timing(i);

		seq_printf(m, "--------------DISP(%d)-------------------------\n", i);
		seq_printf(m, "disp_from_sc(%d)\t\tsync_ext(%d)\t\ttgen_en(%d)\t\tfmt(%s)\n"
			, cfg->disp_from_sc, cfg->sync_ext, cfg->tgen_en, str_disp_fmt[cfg->fmt]);
		seq_printf(m, "in_csc(%15s)\tout_csc(%15s)\tburst(%d)\ty_thresh(%d)\tc_thresh(%d)\n"
			, str_disp_csc[cfg->in_csc], str_disp_csc[cfg->out_csc], cfg->burst, cfg->y_thresh, cfg->c_thresh);
		show_mem(m, &cfg->mem);
		seq_printf(m, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
			, status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
			, status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
			, status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
		seq_printf(m, "bw fail(%d)\n", status.b.bw_fail);
		seq_printf(m, "--------------DISP-TIMING(%d))------------------\n", i);
		seq_printf(m, "total(%4d * %4d)\thsync_pol(%4d)\tvsync_pol(%4d)\n"
			, timing->htotal, timing->vtotal, timing->hsync_pol, timing->vsync_pol);
		seq_printf(m, "hsync_start(%4d)\thsync_end(%4d)\tvsync_start(%4d)\tvsync_end(%4d)\n"
			, timing->htotal, timing->vtotal, timing->vsync_start, timing->vsync_end);
		seq_printf(m, "hde-start(%4d)\t\thde-end(%4d)\tvde-start(%4d)\t\tvde-end(%4d)\t\n"
			, timing->hfde_start, timing->hfde_end, timing->vfde_start, timing->vfde_end);
	}
}

static int vo_disp_proc_show(struct seq_file *m, void *v)
{
	vo_disp_show_disp_status(m);

	return 0;
}

static int vo_disp_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vo_disp_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vo_disp_proc_fops = {
	.proc_open = vo_disp_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vo_disp_proc_fops = {
	.owner = THIS_MODULE,
	.open = vo_disp_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vo_disp_proc_init(void)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VO_DISP_PRC_NAME, 0644, NULL, &_vo_disp_proc_fops, NULL) == NULL) {
		pr_err("vo proc creation failed\n");
		rc = -1;
	}
	return rc;
}

int vo_disp_proc_remove(void)
{
	remove_proc_entry(VO_DISP_PRC_NAME, NULL);

	return 0;
}

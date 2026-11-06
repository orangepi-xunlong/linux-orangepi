#include <linux/version.h>
#include "mipi_tx_proc.h"
#include "../mipi_tx.h"

#define MIPI_TX0_PROC_NAME "soph/mipi_tx0"
#define MIPI_TX1_PROC_NAME "soph/mipi_tx1"
#define MIPI_TX_PROC_MEM_SIZE sizeof(struct combo_dev_cfg_s)

static int mipi_tx_devno;

/*************************************************************************
 *	MIPI_Tx proc functions
 *************************************************************************/
static int mipi_tx_proc_show(struct seq_file *m, void *v)
{
	struct combo_dev_cfg_s mipi_tx_proc_ctx;
	enum mipi_tx_lane_id data_lane_id[LANE_MAX_NUM - 1] = {-1, -1, -1, -1};
	int phy_data_rate = 0, bits_per_pixel = 0;
	int i = 0, data_lane_num = 0;

	mipi_tx_get_combo_dev_cfg(&mipi_tx_proc_ctx, mipi_tx_devno);

	for (i = 0, data_lane_num = 0; i < LANE_MAX_NUM; ++i, ++data_lane_num) {
		if (mipi_tx_proc_ctx.lane_id[i] != MIPI_TX_LANE_CLK &&
		    mipi_tx_proc_ctx.lane_id[i] != -1)
			data_lane_id[mipi_tx_proc_ctx.lane_id[i] - 1] = data_lane_num + 1;
		else
			--data_lane_num;
	}

	switch (mipi_tx_proc_ctx.output_format) {
	case (OUT_FORMAT_RGB_16_BIT):
	case (OUT_FORMAT_YUV422_8_BIT):
		bits_per_pixel = 16;
		break;

	case (OUT_FORMAT_RGB_18_BIT):
	case (OUT_FORMAT_RGB_24_BIT):
		bits_per_pixel = 24;
		break;

	case (OUT_FORMAT_RGB_30_BIT):
		bits_per_pixel = 32;
		break;

	case (OUT_FORMAT_YUV420_8_BIT_NORMAL):
	case (OUT_FORMAT_YUV420_8_BIT_LEGACY):
		bits_per_pixel = 12;
		break;

	default:
		bits_per_pixel = 24;
		break;
	}

	if (data_lane_num == 0)
		memset(data_lane_id, 0, sizeof(data_lane_id));
	else {
		phy_data_rate = mipi_tx_proc_ctx.pixel_clk * bits_per_pixel * 10 / 8 / data_lane_num / 1000;
		phy_data_rate = phy_data_rate % 10 ? phy_data_rate / 10 + 1 : phy_data_rate / 10;
	}

	seq_printf(m, "\nModule: [MIPI_TX], Build Time[%s]\n", UTS_VERSION);
	// MIPI_Tx DEV CONFIG
	seq_printf(m, "\n------MIPI_Tx%d DEV CONFIG----------------------------------------\n",
		   mipi_tx_proc_ctx.devno);
	seq_printf(m, "%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s\n",
		   "devno", "lane0", "lane1", "lane2", "lane3", "output_mode",
		   "phy_data_rate", "pixel_clk(KHz)", "video_mode", "output_fmt");
	seq_printf(m, "%10d%10d%10d%10d%10d%15d%15d%15d%15d%15d\n",
		   mipi_tx_proc_ctx.devno,
		   data_lane_id[0],
		   data_lane_id[1],
		   data_lane_id[2],
		   data_lane_id[3],
		   mipi_tx_proc_ctx.output_mode,
		   phy_data_rate,
		   mipi_tx_proc_ctx.pixel_clk,
		   mipi_tx_proc_ctx.video_mode,
		   mipi_tx_proc_ctx.output_format);

	// MIPI_Tx SYNC CONFIG
	seq_puts(m, "\n------MIPI_Tx SYNC CONFIG---------------------------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s%15s%15s%15s\n",
		   "pkt_size", "hsa_pixels", "hbp_pixels", "hline_pixels", "vsa_lines",
		   "vbp_lines", "vfp_lines", "active_lines", "edpi_cmd_size");
	seq_printf(m, "%15d%15d%15d%15d%15d%15d%15d%15d%15d\n",
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hfp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines,
		   mipi_tx_proc_ctx.sync_info.vid_vbp_lines,
		   mipi_tx_proc_ctx.sync_info.vid_vfp_lines,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines,
		   mipi_tx_proc_ctx.sync_info.edpi_cmd_size);

	// MIPI_Tx DEV STATUS
	seq_puts(m, "\n------MIPI_Tx DEV STATUS----------------------------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%10s%10s%10s\n",
		   "width", "height", "HoriAll", "VertAll", "hbp", "hsa", "vsa");
	seq_printf(m, "%15d%15d%15d%15d%10d%10d%10d\n",
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines,
		   mipi_tx_proc_ctx.sync_info.vid_hline_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels +
		   mipi_tx_proc_ctx.sync_info.vid_hfp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_active_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vfp_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vbp_lines +
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines,
		   mipi_tx_proc_ctx.sync_info.vid_hbp_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_hsa_pixels,
		   mipi_tx_proc_ctx.sync_info.vid_vsa_lines);

	return 0;
}

static int mipi_tx_proc_open(struct inode *inode, struct file *file)
{
	if (strstr(MIPI_TX0_PROC_NAME, file->f_path.dentry->d_iname)) {
		mipi_tx_devno = 0;
	} else if (strstr(MIPI_TX1_PROC_NAME, file->f_path.dentry->d_iname)) {
		mipi_tx_devno = 1;
	}

	return single_open(file, mipi_tx_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops mipi_tx_proc_fops = {
	.proc_open = mipi_tx_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations mipi_tx_proc_fops = {
	.owner = THIS_MODULE,
	.open = mipi_tx_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int mipi_tx_proc_init(int devno)
{
	int rc = 0;

	/* create the /proc file */
	if (devno == 0) {
		if (proc_create_data(MIPI_TX0_PROC_NAME, 0644, NULL, &mipi_tx_proc_fops, NULL) == NULL) {
			pr_err("mipi_tx proc creation failed\n");
			rc = -1;
		}
	} else if (devno == 1) {
		if (proc_create_data(MIPI_TX1_PROC_NAME, 0644, NULL, &mipi_tx_proc_fops, NULL) == NULL) {
			pr_err("mipi_tx proc creation failed\n");
			rc = -1;
		}
	}

	return rc;
}

int mipi_tx_proc_remove(int devno)
{
	if (devno == 0) {
		remove_proc_entry(MIPI_TX0_PROC_NAME, NULL);
	} else if (devno == 1) {
		remove_proc_entry(MIPI_TX1_PROC_NAME, NULL);
	}

	return 0;
}


#include <proc/vi_proc.h>
#include <linux/version.h>
#include <vi_ctx.h>

#define VI_PRC_NAME	"v4l2/vi"

static void *vi_shared_mem;
/*************************************************************************
 *	VI proc functions
 *************************************************************************/

static int _vi_proc_show(struct seq_file *m, void *v)
{
	struct sop_vi_dev *vdev = m->private;
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	u8 i = 0, j = 0, chn = 0;
	char o[8], p[8];
	u8 is_rgb = 0;
	u8 raw_num = 0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vi_shared_mem);

	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------------\n");
	seq_puts(m, "\tDetectErrFrame\tDropErrFrame\n");
	seq_printf(m, "\t\t%d\t\t%d\n", vi_proc_ctx->mod_param.detect_err_frame, vi_proc_ctx->mod_param.drop_err_frame);

	seq_puts(m, "\n-------------------------------VI MODE------------------------------------------\n");
	seq_puts(m, "\tDevID\tPrerawFE\tPrerawBE\tPostraw\t\tScaler\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			raw_num = vi_get_raw_num_by_dev(&vdev->ctx, i);

			seq_printf(m, "\t%3d\t%7s\t\t%7s\t\t%7s\t\t%7s\n", i,
				vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_fe ? "replay_fe" :
				(vdev->ctx.isp_pipe_cfg[raw_num].is_raw_replay_be ? "replay_be" : "online"),
				vdev->ctx.is_offline_be ? "offline" : "online",
				vdev->ctx.is_offline_postraw ?
				(vdev->ctx.is_slice_buf_on ? "slice" : "offline") : "online",
				vdev->ctx.isp_pipe_cfg[raw_num].is_offline_scaler ? "offline" : "online");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR1-------------------------------------\n");
	seq_puts(m, "\tDevID\tDevEn\tBindPipe\tWidth\tHeight\tIntfM\tWkM\tScanM\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			seq_printf(m, "\t%3d\t%3s\t%4s\t\t%4d\t%4d", i,
				(vi_proc_ctx->is_dev_enable[i] ? "Y" : "N"), "Y",
				vi_proc_ctx->dev_attr[i].size.width,
				vi_proc_ctx->dev_attr[i].size.height);

			memset(o, 0, 8);
			if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT656 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT601 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_STANDARD ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_INTERLEAVED)
				strncat(o, "BT", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_NORMAL ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_LEGACY ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV422)
				strncat(o, "MIPI", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_LVDS)
				strncat(o, "LVDS", sizeof(o));

			memset(p, 0, 8);
			if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_1MULTIPLEX)
				strncat(p, "1MUX", sizeof(p));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_2MULTIPLEX)
				strncat(p, "2MUX", sizeof(p));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_3MULTIPLEX)
				strncat(p, "3MUX", sizeof(p));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_4MULTIPLEX)
				strncat(p, "4MUX", sizeof(p));
			else
				strncat(p, "Other", sizeof(p));

			seq_printf(m, "\t%4s\t%4s\t%3s\n", o, p,
				(vi_proc_ctx->dev_attr[i].scan_mode == VI_SCAN_INTERLACED) ? "I" : "P");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR2-------------------------------------\n");
	seq_puts(m, "\tDevID\tAD0\tAD1\tAD2\tAD3\tSeq\tDataType\tWDRMode\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			memset(o, 0, 8);
			if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VUVU)
				strncat(o, "VUVU", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UVUV)
				strncat(o, "UVUV", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UYVY)
				strncat(o, "UYVY", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VYUY)
				strncat(o, "VYUY", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YUYV)
				strncat(o, "YUYV", sizeof(o));
			else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YVYU)
				strncat(o, "YVYU", sizeof(o));

			is_rgb = (vi_proc_ctx->dev_attr[i].input_data_type == VI_DATA_TYPE_RGB);
			raw_num = vi_get_raw_num_by_dev(&vdev->ctx, i);

			seq_printf(m, "\t%3d\t%1d\t%1d\t%1d\t%1d\t%3s\t%4s\t\t%3s\n", i,
				vi_proc_ctx->dev_attr[i].ad_chn_id[0],
				vi_proc_ctx->dev_attr[i].ad_chn_id[1],
				vi_proc_ctx->dev_attr[i].ad_chn_id[2],
				vi_proc_ctx->dev_attr[i].ad_chn_id[3],
				(is_rgb) ? "N/A" : o, (is_rgb) ? "RGB" : "YUV",
				(vdev->ctx.isp_pipe_cfg[raw_num].is_hdr_on) ? "WDR_2F1" : "None");
		}
	}

	seq_puts(m, "\n-------------------------------VI BIND ATTR-------------------------------------\n");
	seq_puts(m, "\tDevID\tPipeNum\t\tPipeId\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			seq_printf(m, "\t%3d\t%3d\t\t%3d\n", i,
				vi_proc_ctx->bind_pipe_attr[i].num,
				vi_proc_ctx->bind_pipe_attr[i].pipe_id[0]);
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV TIMING ATTR-------------------------------\n");
	seq_puts(m, "\tDevID\tDevTimingEn\tDevFrmRate\tDevWidth\tDevHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			seq_printf(m, "\t%3d\t%5s\t\t%4d\t\t%5d\t\t%5d\n", i,
				(vi_proc_ctx->timing_attr[i].enable) ? "Y" : "N",
				vi_proc_ctx->timing_attr[i].frm_rate,
				vi_proc_ctx->dev_attr[i].size.width,
				vi_proc_ctx->dev_attr[i].size.height);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN ATTR1-------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tWidth\tHeight\tMirror\tFlip\tSrcFRate\tDstFRate\tPixFmt\tVideoFmt\tBindPool\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
					strncat(o, "422P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
					strncat(o, "420P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
					strncat(o, "444P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_NV12)
					strncat(o, "NV12", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_NV21)
					strncat(o, "NV21", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%4d\t%4d\t%3s\t%2s\t%4d\t\t%4d\t\t%3s\t%6s\t\t%4d\n", i, j,
					vi_proc_ctx->chn_attr[chn].size.width,
					vi_proc_ctx->chn_attr[chn].size.height,
					(vi_proc_ctx->chn_attr[chn].mirror) ? "Y" : "N",
					(vi_proc_ctx->chn_attr[chn].flip) ? "Y" : "N",
					vi_proc_ctx->chn_attr[chn].frame_rate.src_frame_rate,
					vi_proc_ctx->chn_attr[chn].frame_rate.dst_frame_rate,
					o, "SDR8", vi_proc_ctx->chn_attr[chn].bind_vb_pool);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN ATT2--------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tCompressMode\tDepth\tAlign\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;
				memset(o, 0, 8);
				if (vi_proc_ctx->chn_attr[chn].compress_mode == COMPRESS_MODE_NONE)
					strncat(o, "None", sizeof(o));
				else
					strncat(o, "Y", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%4s\t\t%3d\t%3d\n", i, j,
					o, vi_proc_ctx->chn_attr[chn].depth, 32);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN OUTPUT RESOLUTION-------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tMirror\tFlip\tWidth\tHeight\tPixFmt\tVideoFmt\tCompressMode\tFrameRate\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
					strncat(o, "422P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
					strncat(o, "420P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
					strncat(o, "444P", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_NV12)
					strncat(o, "NV12", sizeof(o));
				else if (vi_proc_ctx->chn_attr[chn].pixel_format == PIXEL_FORMAT_NV21)
					strncat(o, "NV21", sizeof(o));

				memset(p, 0, 8);
				if (vi_proc_ctx->chn_attr[chn].compress_mode == COMPRESS_MODE_NONE)
					strncat(p, "None", sizeof(p));
				else
					strncat(p, "Y", sizeof(p));

				seq_printf(m, "\t%3d\t%3d\t%3s\t%2s\t%4d\t%4d\t%3s\t%6s\t\t%6s\t\t%5d\n", i, j,
					(vi_proc_ctx->chn_attr[chn].mirror) ? "Y" : "N",
					(vi_proc_ctx->chn_attr[chn].flip) ? "Y" : "N",
					vi_proc_ctx->chn_attr[chn].size.width,
					vi_proc_ctx->chn_attr[chn].size.height,
					o, "SDR8", p,
					vi_proc_ctx->chn_attr[chn].frame_rate.dst_frame_rate);
			}
		}
	}


	seq_puts(m, "\n-------------------------------VI CHN ROTATE INFO-------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tRotate\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (vi_proc_ctx->rotation[chn] == ROTATION_0)
					strncat(o, "0", sizeof(o));
				else if (vi_proc_ctx->rotation[chn] == ROTATION_90)
					strncat(o, "90", sizeof(o));
				else if (vi_proc_ctx->rotation[chn] == ROTATION_180)
					strncat(o, "180", sizeof(o));
				else if (vi_proc_ctx->rotation[chn] == ROTATION_270)
					strncat(o, "270", sizeof(o));
				else
					strncat(o, "Invalid", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%3s\n", i, j, o);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN EARLY INTERRUPT INFO----------------------\n");
	seq_puts(m, "\tDevID\tChnID\tEnable\tLineCnt\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				seq_printf(m, "\t%3d\t%3d\t%3s\t%4d\n", i, j,
					vi_proc_ctx->ealy_int[chn].enable ? "Y" : "N",
					vi_proc_ctx->ealy_int[chn].line_cnt);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN CROP INFO---------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tCropEn\tCoorType\tCoorX\tCoorY\tWidth\tHeight\tTrimX\tTrimY\tTrimWid\tTrimHgt\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				memset(o, 0, 8);
				if (vi_proc_ctx->chn_crop[chn].crop_coordinate == VI_CROP_RATIO_COOR)
					strncat(o, "RAT", sizeof(o));
				else
					strncat(o, "ABS", sizeof(o));

				seq_printf(m, "\t%3d\t%3d\t%3s\t%5s\t\t%4d\t%4d\t%4d\t%4d\t%4d\t%3d\t%3d\t%4d\n", i, j,
					vi_proc_ctx->chn_crop[chn].enable ? "Y" : "N", o,
					vi_proc_ctx->chn_crop[chn].crop_rect.x,
					vi_proc_ctx->chn_crop[chn].crop_rect.y,
					vi_proc_ctx->chn_crop[chn].crop_rect.width,
					vi_proc_ctx->chn_crop[chn].crop_rect.height,
					vi_proc_ctx->chn_crop[chn].crop_rect.x,
					vi_proc_ctx->chn_crop[chn].crop_rect.y,
					vi_proc_ctx->chn_crop[chn].crop_rect.width,
					vi_proc_ctx->chn_crop[chn].crop_rect.height);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN STATUS------------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tEnable\tFrameRate\tIntCnt\tRecvPic\tLostFrame\tVbFail\tWidth\tHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			for (chn = 0, j = 0; j < i; j++) {
				chn += vi_proc_ctx->dev_attr[j].chn_num;
			}

			for (j = 0; j < vi_proc_ctx->dev_attr[i].chn_num; j++, chn++) {
				if (chn >= vi_proc_ctx->total_chn_num)
					break;

				seq_printf(m, "\t%3d\t%3d\t%3s\t%5d\t\t%5d\t%5d\t%5d\t\t%5d\t%4d\t%4d\n", i, j,
					vi_proc_ctx->chn_status[chn].enable ? "Y" : "N",
					vi_proc_ctx->chn_status[chn].frame_rate,
					vi_proc_ctx->chn_status[chn].int_cnt,
					vi_proc_ctx->chn_status[chn].recv_pic,
					vi_proc_ctx->chn_status[chn].lost_frame,
					vi_proc_ctx->chn_status[chn].vb_fail,
					vi_proc_ctx->chn_status[chn].size.width,
					vi_proc_ctx->chn_status[chn].size.height);
			}
		}
	}

	return 0;
}

static int _vi_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vi_proc_show, PDE_DATA(inode));
}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops _vi_proc_fops = {
	.proc_open = _vi_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vi_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vi_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_proc_init(struct sop_vi_dev *_vdev, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_PRC_NAME, 0644, NULL, &_vi_proc_fops, _vdev) == NULL) {
		pr_err("vi proc creation failed\n");
		rc = -1;
	}

	vi_shared_mem = shm;
	return rc;
}

int vi_proc_remove(void)
{
	remove_proc_entry(VI_PRC_NAME, NULL);

	return 0;
}

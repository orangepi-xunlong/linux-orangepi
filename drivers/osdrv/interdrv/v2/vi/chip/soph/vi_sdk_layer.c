#include <linux/slab.h>
#include <linux/uaccess.h>

#include <vi_sdk_layer.h>
#include <base_ctx.h>
#include <linux/comm_buffer.h>
#include <vi_ctx.h>
#include <linux/defines.h>
#include <linux/comm_errno.h>
#include <ldc_cb.h>
#include <vi_raw_dump.h>
#include "base_common.h"
#include "sys.h"
#include "vbq.h"
#include "ion.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/

extern struct sop_vi_ctx *g_vi_ctx;
extern struct gdc_mesh g_vi_mesh[VI_MAX_CHN_NUM];
static struct sop_vi_dev *gvdev;
static struct mlv_i_s g_mlv_i[VI_MAX_DEV_NUM];

static struct crop_size_s dis_i_data[VI_MAX_DEV_NUM] = { 0 };
static u32 dis_i_frm_num[VI_MAX_DEV_NUM] = { 0 };
static u32 dis_flag[VI_MAX_DEV_NUM] = { 0 };
static u32 dev_bind_pipe_attr[VI_MAX_DEV_NUM] = {0};
static wait_queue_head_t dis_wait_q[VI_MAX_DEV_NUM];
struct vb_jobs_t vi_jobs[VI_MAX_CHN_NUM];

static inline int check_vi_dev_valid(int dev)
{
	if (dev > (VI_MAX_DEV_NUM - 1) || dev < 0) {
		vi_pr(VI_ERR, "dev num expect 0~%d, but now %d Caller is %p\n",
		      VI_MAX_DEV_NUM - 1, dev, __builtin_return_address(0));
		return ERR_VI_INVALID_DEVID;
	}

	return 0;
}

static inline int check_vi_chn_valid(int chn)
{
	if (chn > (VI_MAX_CHN_NUM - 1) || chn < 0) {
		vi_pr(VI_ERR, "chn num expect 0~%d, but now %d Caller is %p\n",
			VI_MAX_CHN_NUM - 1, chn, __builtin_return_address(0));
		return ERR_VI_INVALID_CHNID;
	}

	return 0;
}

static inline int check_vi_chn_enable(int chn)
{
	if (!g_vi_ctx->is_chn_enable[chn]) {
		vi_pr(VI_ERR, "chn %d, not created Caller is %p\n",
			chn, __builtin_return_address(0));
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	return 0;
}

static inline int check_vi_pipe_valid(int pipe)
{
	if (pipe < 0 || pipe > (VI_MAX_PIPE_NUM - 1)) {
		vi_pr(VI_ERR, "pipe num expect 0~%d, but now %d Caller is %p\n",
			VI_MAX_PIPE_NUM - 1, pipe, __builtin_return_address(0));
		return ERR_VI_INVALID_PIPEID;
	}

	return 0;
}

static inline int check_vi_pipe_created(int pipe)
{
	if (!g_vi_ctx->is_pipe_created[pipe]) {
		vi_pr(VI_ERR, "pipe %d not created Caller is %p\n",
			pipe, __builtin_return_address(0));
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	return 0;
}

static inline int check_vi_size_valid(int w, int h)
{
	if (w > VI_DEV_MAX_WIDTH || h > VI_DEV_MAX_HEIGHT) {
		vi_pr(VI_ERR, "size is too large w:%d h:%d Caller is %p\n",
			w, h, __builtin_return_address(0));
		return ERR_VI_INVALID_PARA;
	}

	return 0;
}

/****************************************************************************
 * SDK layer APIs
 ****************************************************************************/
static int _vi_sdk_drv_qbuf(struct video_buffer *buf, int raw_id, int chn_id)
{
	struct sop_isp_buf *qbuf;
	u8 i = 0;

	qbuf = kzalloc(sizeof(struct sop_isp_buf), GFP_ATOMIC);
	if (qbuf == NULL) {
		vi_pr(VI_ERR, "qbuf kzalloc size(%zu) failed\n", sizeof(struct sop_isp_buf));
		return -ENOMEM;
	}

	qbuf->buf.index = raw_id * VI_MAX_CHN_NUM + chn_id;
	switch (buf->pixel_format) {
	default:
	case PIXEL_FORMAT_YUV_PLANAR_420:
	case PIXEL_FORMAT_YUV_PLANAR_422:
	case PIXEL_FORMAT_YUV_PLANAR_444:
		qbuf->buf.length = 3;
		break;
	case PIXEL_FORMAT_NV21:
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV61:
	case PIXEL_FORMAT_NV16:
		qbuf->buf.length = 2;
		break;
	case PIXEL_FORMAT_YUV_400:
	case PIXEL_FORMAT_YUYV:
	case PIXEL_FORMAT_YVYU:
	case PIXEL_FORMAT_UYVY:
	case PIXEL_FORMAT_VYUY:
		qbuf->buf.length = 1;
		break;
	}

	for (i = 0; i < qbuf->buf.length; i++) {
		qbuf->buf.planes[i].addr = buf->phy_addr[i];
	}

	sop_isp_rdy_buf_queue(gvdev, qbuf);

	return 0;
}

static int vi_set_dev_bind_info(int vi_dev, vi_dev_bind_pipe_s *dev_bind_attr)
{
	struct isp_ctx *ctx = &gvdev->ctx;
	int raw_num = 0;
	int mac_num = 0;
	int ret = -1;

	if (!dev_bind_attr->num) {
		vi_pr(VI_ERR, "no bind attr have been set\n");
		return ret;
	}

	if (dev_bind_attr->pipe_id[0] >= VI_MAX_DEV_NUM) {
		vi_pr(VI_ERR, "invalid mac(%d)\n", dev_bind_attr->pipe_id[0]);
		return ret;
	}

	mac_num = dev_bind_attr->pipe_id[0];
	raw_num = mac_num;

#if defined(__CV186X__)
	if (mac_num == ISP_PRERAW1)
		raw_num = ISP_PRERAW3;
	else if (mac_num == ISP_PRERAW3)
		raw_num = ISP_PRERAW1;
#endif

	if (ctx->isp_bind_info[raw_num].is_bind) {
		vi_pr(VI_ERR, "mac(%d) is binded before, set dev unbind first\n", mac_num);
		return ret;
	}

	if ((g_vi_ctx->dev_attr[vi_dev].input_data_type == VI_DATA_TYPE_YUV ||
	     g_vi_ctx->dev_attr[vi_dev].input_data_type == VI_DATA_TYPE_YUV_EARLY) &&
	      (g_vi_ctx->dev_attr[vi_dev].work_mode > 0 &&
	       g_vi_ctx->dev_attr[vi_dev].intf_mode >= VI_MODE_BT656 &&
	       g_vi_ctx->dev_attr[vi_dev].intf_mode <= VI_MODE_BT1120_INTERLEAVED)) { // bt_demux
		if (!(raw_num >= ISP_PRERAW_LITE0 && raw_num <= ISP_PRERAW_LITE1)) {
			vi_pr(VI_ERR, "invalid bind mac(%d) from vi dev(%d)\n", mac_num, vi_dev);
			return ret;
		}
	} else {
		if (!(raw_num >= ISP_PRERAW0 && raw_num <= ISP_PRERAW5)) {
			vi_pr(VI_ERR, "invalid bind mac(%d) from vi dev(%d)\n", mac_num, vi_dev);
			return ret;
		}
	}

	vi_pr(VI_INFO, "dev(%d) raw_num_%d bind to mac(%d)\n", vi_dev, raw_num, mac_num);
	ctx->isp_bind_info[raw_num].is_bind = true;
	ctx->isp_bind_info[raw_num].bind_dev_num = vi_dev;
	ret = 0;

	return ret;
}

int vi_sdk_qbuf(mmf_chn_s chn, void *data)
{
	struct isp_ctx *ctx = &gvdev->ctx;
	int buf_chn = ctx->raw_chnstr_num[chn.dev_id] + chn.chn_id;

	vb_blk blk = vb_get_block_with_id(g_vi_ctx->chn_attr[buf_chn].bind_vb_pool,
					g_vi_ctx->blk_size[buf_chn], ID_VI);
	size_s size = g_vi_ctx->chn_attr[buf_chn].size;
	int rc = 0;

	if (blk == VB_INVALID_HANDLE) {
		g_vi_ctx->chn_status[buf_chn].vb_fail++;
		vi_pr(VI_DBG, "Can't acquire VB BLK for VI, size(%d)\n", g_vi_ctx->blk_size[buf_chn]);
		return -ENOMEM;
	}

	// workaround for ldc 64-align for width/height.
	if (g_vi_ctx->rotation[buf_chn] != ROTATION_0 || g_vi_ctx->ldc_attr[buf_chn].enable) {
		size.width = ALIGN(size.width, LDC_ALIGN);
		size.height = ALIGN(size.height, LDC_ALIGN);
	}

	base_get_frame_info(g_vi_ctx->chn_attr[buf_chn].pixel_format
			   , size
			   , &((struct vb_s *)blk)->buf
			   , vb_handle2phys_addr(blk)
			   , DEFAULT_ALIGN);

	((struct vb_s *)blk)->buf.offset_top = 0;
	((struct vb_s *)blk)->buf.offset_right = size.width - g_vi_ctx->chn_attr[buf_chn].size.width;
	((struct vb_s *)blk)->buf.offset_left = 0;
	((struct vb_s *)blk)->buf.offset_bottom = size.height - g_vi_ctx->chn_attr[buf_chn].size.height;

	rc = vb_qbuf(chn, CHN_TYPE_OUT, &vi_jobs[buf_chn], blk);
	if (rc != 0) {
		vi_pr(VI_ERR, "vb_qbuf failed\n");
		return rc;
	}

	rc = _vi_sdk_drv_qbuf(&((struct vb_s *)blk)->buf, chn.dev_id, chn.chn_id);
	if (rc != 0) {
		vi_pr(VI_ERR, "_vi_sdk_drv_qbuf failed\n");
		return rc;
	}

	rc = vb_release_block(blk);
	if (rc != 0) {
		vi_pr(VI_ERR, "vb_release_block failed\n");
		return rc;
	}

	return rc;
}

void vi_fill_mlv_info(struct vb_s *blk, u8 dev, struct mlv_i_s *m_lv_i, u8 is_vpss_offline)
{
	if (is_vpss_offline) {
		u8 snr_num = dev;

		blk->buf.motion_lv = g_mlv_i[snr_num].mlv_i_level;
		memcpy(blk->buf.motion_table, g_mlv_i[snr_num].mlv_i_table, MO_TBL_SIZE);
	} else {
		u8 snr_num = dev;

		m_lv_i->mlv_i_level = g_mlv_i[snr_num].mlv_i_level;
		memcpy(m_lv_i->mlv_i_table, g_mlv_i[snr_num].mlv_i_table, MO_TBL_SIZE);
	}
}

int vi_set_motion_lv(struct mlv_info_s mlv_i)
{
	if (mlv_i.sensor_num >= VI_MAX_DEV_NUM)
		return -1;

	g_mlv_i[mlv_i.sensor_num].mlv_i_level = mlv_i.mlv;
	memcpy(g_mlv_i[mlv_i.sensor_num].mlv_i_table, mlv_i.mtable, MO_TBL_SIZE);

	return 0;
}

void vi_fill_dis_info(struct vb_s *blk, u8 dev)
{
	u8 snr_num = dev;
	u32 frm_num = blk->buf.frm_num;
	u8 is_set_dis_info = false;
	static struct crop_size_s dis_i[VI_MAX_DEV_NUM] = { 0 };

	if (g_vi_ctx->is_dis_enable[snr_num]) {
		if (!wait_event_timeout(dis_wait_q[snr_num], dis_flag[snr_num] == 1, msecs_to_jiffies(10))) {
			vi_pr(VI_ERR, "snr(%d) frm(%d) timeout\n", snr_num, frm_num);
		}

		if (dis_flag[snr_num] == 1) {
			if (dis_i_frm_num[snr_num] == frm_num) {
				dis_i[snr_num] = dis_i_data[snr_num];
				is_set_dis_info = true;
			}

			vi_pr(VI_DBG, "is_set_dis_info(%d) fill dis snr(%d), frm(%d), crop(%d, %d, %d, %d)\n"
				, is_set_dis_info
				, snr_num
				, frm_num
				, dis_i[snr_num].start_x
				, dis_i[snr_num].start_y
				, dis_i[snr_num].end_x
				, dis_i[snr_num].end_y);
		}

		dis_flag[snr_num] = 0;

		blk->buf.frame_crop.start_x = dis_i[snr_num].start_x;
		blk->buf.frame_crop.start_y = dis_i[snr_num].start_y;
		blk->buf.frame_crop.end_x = dis_i[snr_num].end_x;
		blk->buf.frame_crop.end_y = dis_i[snr_num].end_y;
	}
}

int vi_set_dis_info(struct dis_info_s dis_i)
{
	u32 frm_num = dis_i.frm_num;
	u8 snr_num = dis_i.sensor_num;
	struct crop_size_s crop = dis_i.dis_i;

	if (dis_i.sensor_num >= VI_MAX_DEV_NUM) {
		vi_pr(VI_ERR, "invalid sensor_num(%d)\n", dis_i.sensor_num);
		return -1;
	}

	vi_pr(VI_DBG, "set dis snr(%d), frm(%d), crop(%d, %d, %d, %d)\n"
			, snr_num
			, frm_num
			, crop.start_x
			, crop.start_y
			, crop.end_x
			, crop.end_y);

	dis_i_frm_num[snr_num] = frm_num;
	dis_i_data[snr_num] = crop;

	if (wq_has_sleeper(&dis_wait_q[snr_num])) {
		dis_flag[snr_num] = 1;
		wake_up(&dis_wait_q[snr_num]);
	}

	return 0;
}

int vi_set_bypass_frm(int pipe, u8 bypass_num)
{
	int ret = 0;

	ret = check_vi_pipe_valid(pipe);
	if (ret != 0)
		return ret;

	g_vi_ctx->bypass_frm[pipe] = bypass_num;

	return 0;
}

int vi_set_dev_attr(int vi_dev, const vi_dev_attr_s *dev_attr)
{
	int ret = 0;
	u32 chn_num = 1;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	memcpy(&g_vi_ctx->dev_attr[vi_dev], dev_attr, sizeof(vi_dev_attr_s));

	if (dev_attr->input_data_type == VI_DATA_TYPE_YUV) {
		switch (dev_attr->work_mode) {
		case VI_WORK_MODE_1MULTIPLEX:
			chn_num = 1;
			break;
		case VI_WORK_MODE_2MULTIPLEX:
			chn_num = 2;
			break;
		case VI_WORK_MODE_3MULTIPLEX:
			chn_num = 3;
			break;
		case VI_WORK_MODE_4MULTIPLEX:
			chn_num = 4;
			break;
		default:
			vi_pr(VI_ERR, "SNR work mode(%d) is wrong\n", dev_attr->work_mode);
			return -1;
		}
	}

	g_vi_ctx->dev_attr[vi_dev].chn_num = chn_num;

	vi_pr(VI_DBG, "dev=%d chn_num=%d\n", vi_dev, chn_num);

	return ret;
}

int vi_get_dev_attr(int vi_dev, vi_dev_attr_s *dev_attr)
{
	int ret = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	*dev_attr = g_vi_ctx->dev_attr[vi_dev];

	return ret;
}

int vi_set_dev_bind_attr(int vi_dev, const vi_dev_bind_pipe_s *dev_bind_attr)
{
	int ret = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	memcpy(&g_vi_ctx->bind_pipe_attr[vi_dev], dev_bind_attr, sizeof(vi_dev_bind_pipe_s));

#ifndef FPGA_PORTING
	dev_bind_pipe_attr[vi_dev] = g_vi_ctx->bind_pipe_attr[vi_dev].pipe_id[0];
#endif

	return ret;
}

int vi_get_dev_bind_attr(int vi_dev, vi_dev_bind_pipe_s *dev_bind_attr)
{
	int ret = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	*dev_bind_attr = g_vi_ctx->bind_pipe_attr[vi_dev];

	return ret;
}

int vi_set_dev_unbind_attr(int vi_dev)
{
	struct isp_ctx *ctx = &gvdev->ctx;
	int raw_num = 0;
	int mac_num = 0;
	int ret = 0;

    ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_dev_enable[vi_dev]) {
		vi_pr(VI_DBG, "vi dev(%d) is already enabled, cannot set unbind.", vi_dev);
		return 0;
	}

	mac_num = dev_bind_pipe_attr[vi_dev];
	raw_num = mac_num;

#if defined(__CV186X__)
	if (mac_num == ISP_PRERAW1)
		raw_num = ISP_PRERAW3;
	else if (mac_num == ISP_PRERAW3)
		raw_num = ISP_PRERAW1;
#endif

	ctx->isp_bind_info[raw_num].is_bind = false;

#ifndef FPGA_PORTING
	dev_bind_pipe_attr[vi_dev] = 0;
#endif

    return ret;
}

int vi_enable_dev(int vi_dev)
{
	int ret = 0;
	int raw_num = ISP_PRERAW0;
	struct isp_ctx *ctx = &gvdev->ctx;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->dev_attr[vi_dev].size.width == 0 &&
	    g_vi_ctx->dev_attr[vi_dev].size.height == 0) {
		vi_pr(VI_ERR, "Call Setdev_attr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	ret = vi_set_dev_bind_info(vi_dev, &g_vi_ctx->bind_pipe_attr[vi_dev]);
	if (ret != 0) {
		vi_pr(VI_ERR, "bind failed\n");
		return ERR_VI_INVALID_PARA;
	}

	raw_num = vi_get_raw_num_by_dev(ctx, (u8)vi_dev);

	// Now from VI_CB_SET_VIVPSSMODE callback
	ctx->isp_pipe_cfg[raw_num].is_offline_scaler = ctx->isp_pipe_offline_sc[(u8)vi_dev];

	if (g_vi_ctx->dev_attr[vi_dev].input_data_type == VI_DATA_TYPE_YUV ||
	    g_vi_ctx->dev_attr[vi_dev].input_data_type == VI_DATA_TYPE_YUV_EARLY) {
		ctx->isp_pipe_cfg[raw_num].is_yuv_sensor	= true;
		ctx->isp_pipe_cfg[raw_num].inf_mode		= g_vi_ctx->dev_attr[vi_dev].intf_mode;
		ctx->isp_pipe_cfg[raw_num].mux_mode		= g_vi_ctx->dev_attr[vi_dev].work_mode;
		ctx->isp_pipe_cfg[raw_num].data_seq		= g_vi_ctx->dev_attr[vi_dev].data_seq;
		ctx->isp_pipe_cfg[raw_num].yuv_scene_mode	= (enum isp_yuv_scene_e)g_vi_ctx->dev_attr[vi_dev].yuv_scene_mode;
	}

	if (g_vi_ctx->dev_attr[vi_dev].intf_mode == VI_MODE_LVDS) {
		ctx->is_sublvds_path = true;
		vi_pr(VI_WARN, "SUBLVDS_PATH_ON(%d)\n", ctx->is_sublvds_path);
	}

	dis_flag[vi_dev] = 0;
	init_waitqueue_head(&dis_wait_q[vi_dev]);

	ctx->isp_pipe_enable[raw_num] = true;
	g_vi_ctx->is_dev_enable[vi_dev] = true;
	g_vi_ctx->total_dev_num++;

#ifndef FPGA_PORTING
	vi_mac_clk_ctrl(gvdev, g_vi_ctx->bind_pipe_attr[vi_dev].pipe_id[0], true);
	if (g_vi_ctx->dev_attr[vi_dev].size.width > 4608) {
		vi_mac_clk_ctrl(gvdev, ISP_PRERAW3, true);
	}
#endif
	vi_pr(VI_DBG, "dev_%d, raw_num(%d) enable=%d, total_dev_num=%d\n",
		vi_dev, raw_num, g_vi_ctx->is_dev_enable[vi_dev], g_vi_ctx->total_dev_num);

	return 0;
}

int vi_disable_dev(int vi_dev)
{
	int ret = 0;
	u8 i = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_dev_enable[vi_dev] == 0) {
		vi_pr(VI_DBG, "vi dev(%d) is already disabled.", vi_dev);
		return 0;
	}

#ifndef FPGA_PORTING
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (g_vi_ctx->bind_pipe_attr[i].num) {
			vi_mac_clk_ctrl(gvdev, g_vi_ctx->bind_pipe_attr[i].pipe_id[0], false);
			memset(&g_vi_ctx->bind_pipe_attr[i], 0, sizeof(vi_dev_bind_pipe_s));
			if (g_vi_ctx->dev_attr[vi_dev].size.width > 4608) {
				vi_mac_clk_ctrl(gvdev, ISP_PRERAW3, false);
			}
		}
	}
#endif

	g_vi_ctx->is_dev_enable[vi_dev] = false;
	memset(&g_vi_ctx->dev_attr[vi_dev], 0, sizeof(vi_dev_attr_s));

	for (i = 0; i < g_vi_ctx->total_dev_num; i++) {
		if (g_vi_ctx->is_dev_enable[i])
			return ret;
	}

	memset(g_vi_ctx, 0, sizeof(struct sop_vi_ctx));

	return ret;
}

int vi_create_pipe(int vi_pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_pipe_created[vi_pipe] == 1) {
		vi_pr(VI_ERR, "Pipe(%d) has been created\n", vi_pipe);
		return ERR_VI_PIPE_EXIST;
	}

	ret = check_vi_size_valid(pipe_attr->max_width, pipe_attr->max_height);
	if (ret != 0)
		return ret;

	//Clear pipe_attr first.
	memset(&g_vi_ctx->pipe_attr[vi_pipe], 0, sizeof(g_vi_ctx->pipe_attr[vi_pipe]));

	g_vi_ctx->is_pipe_created[vi_pipe] = true;
	if (!(g_vi_ctx->source[vi_pipe] == VI_PIPE_FRAME_SOURCE_USER_FE ||
	      g_vi_ctx->source[vi_pipe] == VI_PIPE_FRAME_SOURCE_USER_BE))
		g_vi_ctx->source[vi_pipe] = VI_PIPE_FRAME_SOURCE_DEV;
	g_vi_ctx->pipe_attr[vi_pipe] = *pipe_attr;

	vi_pr(VI_DBG, "pipeCreated=%d\n", g_vi_ctx->is_pipe_created[vi_pipe]);

	return 0;
}

int vi_destroy_pipe(int vi_pipe)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	if (!g_vi_ctx->is_pipe_created[vi_pipe]) {
		vi_pr(VI_INFO, "Pipe(%d) has been not created\n", vi_pipe);
		return 0;
	}

	memset(&g_vi_ctx->pipe_attr[vi_pipe], 0, sizeof(vi_pipe_attr_s));
	memset(&g_vi_ctx->dump_attr[vi_pipe], 0, sizeof(vi_dump_attr_s));
	memset(&g_vi_ctx->pipe_crop[vi_pipe], 0, sizeof(crop_info_s));
	g_vi_ctx->is_pipe_created[vi_pipe] = false;

	return 0;
}

int vi_start_pipe(int vi_pipe)
{
	int ret = 0;
	struct isp_ctx *ctx = &gvdev->ctx;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->pipe_attr[0].compress_mode == COMPRESS_MODE_TILE) {
		ctx->is_dpcm_on = true;
		vi_pr(VI_WARN, "ISP_COMPRESS_ON(%d)\n", ctx->is_dpcm_on);
	}

	vi_pr(VI_DBG, "start_pipe\n");

	return ret;
}

int vi_set_chn_attr(int vi_pipe, int vi_chn, vi_chn_attr_s *chn_attr)
{
	int ret = 0;
	vb_cal_config_s vb_cal_config;
	u32 chn_num = 1;
	u8 chn_str = g_vi_ctx->total_chn_num;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	if (chn_attr->pixel_format == PIXEL_FORMAT_YUV_PLANAR_420 ||
	    chn_attr->pixel_format == PIXEL_FORMAT_YUV_PLANAR_422) //RGB sensor or YUV sensor
		chn_attr->pixel_format = PIXEL_FORMAT_NV21;

	chn_attr->bind_vb_pool = VB_INVALID_POOLID;
	chn_num = g_vi_ctx->total_chn_num + g_vi_ctx->dev_attr[vi_chn].chn_num;

	for (; chn_str < chn_num; chn_str++) {
		g_vi_ctx->chn_attr[chn_str] = *chn_attr;

		common_getpicbufferconfig(chn_attr->size.width, chn_attr->size.height,
					  chn_attr->pixel_format, DATA_BITWIDTH_8,
					  COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

		g_vi_ctx->blk_size[chn_str] = vb_cal_config.vb_size;
	}

	g_vi_ctx->total_chn_num += g_vi_ctx->dev_attr[vi_chn].chn_num;

	vi_pr(VI_DBG, "total_chn_num=%d, blk_size=%d\n", g_vi_ctx->total_chn_num, vb_cal_config.vb_size);

	return ret;
}

int vi_get_chn_attr(int vi_pipe, int vi_chn, vi_chn_attr_s *chn_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	*chn_attr = g_vi_ctx->chn_attr[vi_chn];

	return ret;
}

int vi_set_dev_timing_attr(int vi_dev, const vi_dev_timing_attr_s *timing_attr)
{
	int ret = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	g_vi_ctx->timing_attr[vi_dev] = *timing_attr;

	gvdev->usr_pic_delay = 0;

	if (timing_attr->enable) {
		if (timing_attr->frm_rate > 30)
			gvdev->usr_pic_delay = msecs_to_jiffies(33);
		else if (timing_attr->frm_rate > 0)
			gvdev->usr_pic_delay = msecs_to_jiffies(1000 / timing_attr->frm_rate);
	}

	if (!gvdev->usr_pic_delay)
		usr_pic_time_remove();

	return ret;
}

int vi_get_dev_timing_attr(int vi_dev, vi_dev_timing_attr_s *timing_attr)
{
	int ret = 0;

	ret = check_vi_dev_valid(vi_dev);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_dev_enable[vi_dev] == 0) {
		vi_pr(VI_ERR, "EnableDev first\n");
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	if (g_vi_ctx->timing_attr[vi_dev].enable == 0) {
		vi_pr(VI_DBG, "SetDevTimingAttr auto trig disable\n");
	}

	*timing_attr = g_vi_ctx->timing_attr[vi_dev];

	return ret;
}

int vi_get_pipe_status(int vi_pipe, vi_pipe_status_s *pipe_status)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;
	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	pipe_status->enable = g_vi_ctx->is_chn_enable[vi_pipe];
	pipe_status->size.height = g_vi_ctx->pipe_attr[vi_pipe].max_height;
	pipe_status->size.width = g_vi_ctx->pipe_attr[vi_pipe].max_width;
	pipe_status->frame_rate = g_vi_ctx->pipe_attr[vi_pipe].frame_rate.src_frame_rate;

	return ret;
}

int vi_get_chn_status(int vi_chn, vi_chn_status_s *pipe_status)
{
	int ret = 0;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_chn);
	if (ret != 0)
		return ret;

	*pipe_status = g_vi_ctx->chn_status[vi_chn];

	return ret;
}

int vi_set_pipe_frame_source(int vi_pipe, const vi_pipe_frame_source_e source)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	g_vi_ctx->source[vi_pipe] = source;
	gvdev->isp_source = (u32)source;
	gvdev->ctx.isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_fe = (gvdev->isp_source == ISP_SOURCE_FE);
	gvdev->ctx.isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be = (gvdev->isp_source == ISP_SOURCE_BE);

	vi_pr(VI_INFO, "isp_source=%d\n", gvdev->isp_source);
	vi_pr(VI_INFO, "ISP_PRERAW0.is_raw_replay_fe=%d, ISP_PRERAW0.is_raw_replay_be=%d\n",
			gvdev->ctx.isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_fe,
			gvdev->ctx.isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_be);

	return ret;
}

int vi_get_pipe_frame_source(int vi_pipe, vi_pipe_frame_source_e *psource)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	*psource = g_vi_ctx->source[vi_pipe];

	return ret;
}

int vi_send_pipe_raw(int vi_pipe, const video_frame_info_s *pvideo_frame)
{
	int ret = 0;
	struct isp_ctx *ctx = &gvdev->ctx;
	u64 phy_addr;
	u32 dmaid_le, dmaid_se;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->source[vi_pipe] == VI_PIPE_FRAME_SOURCE_DEV) {
		vi_pr(VI_ERR, "Pipe(%d) source(%d) incorrect.\n", vi_pipe, g_vi_ctx->source[vi_pipe]);
		return -1;
	}

	if (IS_FRAME_OFFSET_INVALID(pvideo_frame->video_frame)) {
		vi_pr(VI_ERR, "Pipe(%d) frame size (%d %d) offset (%d %d %d %d) invalid\n",
			vi_pipe, pvideo_frame->video_frame.width, pvideo_frame->video_frame.height,
			pvideo_frame->video_frame.offset_left, pvideo_frame->video_frame.offset_right,
			pvideo_frame->video_frame.offset_top, pvideo_frame->video_frame.offset_bottom);
		return ERR_VI_INVALID_PARA;
	}

	if (g_vi_ctx->source[vi_pipe] == VI_PIPE_FRAME_SOURCE_USER_FE) {
		dmaid_le = ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_LE;
		dmaid_se = ISP_BLK_ID_DMA_CTL_SPLT_FE0_RDMA_SE;
		// TODO
		// tile mode
	} else if (g_vi_ctx->source[vi_pipe] == VI_PIPE_FRAME_SOURCE_USER_BE) {
		dmaid_le = ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE;
		dmaid_se = ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE;
	}

	if (pvideo_frame->video_frame.dynamic_range == DYNAMIC_RANGE_HDR10) {
		ctx->is_hdr_on = true;
		ctx->isp_pipe_cfg[ISP_PRERAW0].is_hdr_on = true;
		vi_pr(VI_INFO, "HDR_ON(%d) for raw replay\n", ctx->is_hdr_on);
	}

	gvdev->usr_fmt.width	= pvideo_frame->video_frame.width;
	gvdev->usr_fmt.height	= pvideo_frame->video_frame.height;
	gvdev->usr_fmt.code	= pvideo_frame->video_frame.bayer_format;
	gvdev->usr_crop.left	= pvideo_frame->video_frame.offset_left;
	gvdev->usr_crop.top	= pvideo_frame->video_frame.offset_top;
	gvdev->usr_crop.width	= (pvideo_frame->video_frame.offset_right != 0)
					? pvideo_frame->video_frame.offset_right
					: pvideo_frame->video_frame.width;
	gvdev->usr_crop.height	= (pvideo_frame->video_frame.offset_bottom != 0)
					? pvideo_frame->video_frame.offset_bottom
					: pvideo_frame->video_frame.height;

	if (gvdev->usr_pic_delay) {
		if (pvideo_frame->video_frame.phyaddr[0] == 0) {
			vi_pr(VI_ERR, "auto replay le buf_addr is 0, The current operation trig failed\n");
			return ret;
		}
	} else if (!gvdev->usr_pic_delay) {
		if (pvideo_frame->video_frame.phyaddr[0] == 0) {
			vi_pr(VI_ERR, "manual replay le buf_addr is 0, The current operation trig failed\n");
			return ret;
		}
	}

	phy_addr = pvideo_frame->video_frame.phyaddr[0];
	ispblk_dma_setaddr(ctx, vi_pipe, dmaid_le, phy_addr);
	gvdev->usr_pic_phy_addr[ISP_RAW_PATH_LE] = phy_addr;
	vi_pr(VI_INFO, "raw_replay le(0x%llx)\n", gvdev->usr_pic_phy_addr[ISP_RAW_PATH_LE]);

	if (ctx->is_hdr_on || pvideo_frame->video_frame.pixel_format) {
		phy_addr = pvideo_frame->video_frame.phyaddr[1];
		ispblk_dma_setaddr(ctx, vi_pipe, dmaid_se, phy_addr);
		gvdev->usr_pic_phy_addr[ISP_RAW_PATH_SE] = phy_addr;
		vi_pr(VI_INFO, "raw_replay se(0x%llx)\n", gvdev->usr_pic_phy_addr[ISP_RAW_PATH_SE]);
	}

	if (gvdev->usr_pic_delay) {
		usr_pic_timer_init(gvdev);
	} else {
		user_pic_trig(gvdev);
	}

	return ret;
}

int vi_enable_chn(int vi_chn)
{
	int rc = 0, i = 0;
	struct isp_ctx *ctx = &gvdev->ctx;
	enum sop_isp_raw raw_num = ISP_PRERAW0;
	u8 create_thread = false;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num]) {
			continue;
		}

		if (_is_right_tile(ctx, raw_num))
			continue;

		if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
			create_thread = true;
		}

		for (i = 0;
			i < (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor ? (ctx->isp_pipe_cfg[raw_num].mux_mode + 1) : 1);
			i++) {
			u8 num_buffers = 0;
			mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = raw_num, .chn_id = i};

			if (i == 0)
				num_buffers = VI_CHN_0_BUF;
			else if (i == 1)
				num_buffers = VI_CHN_1_BUF;
			else if (i == 2)
				num_buffers = VI_CHN_2_BUF;
			else
				num_buffers = VI_CHN_3_BUF;

			vi_chn = ctx->raw_chnstr_num[raw_num] + i;
			g_vi_ctx->is_chn_enable[vi_chn] = 1;
			g_vi_ctx->chn_status[vi_chn].enable = 1;
			g_vi_ctx->chn_status[vi_chn].size.width = g_vi_ctx->chn_attr[vi_chn].size.width;
			g_vi_ctx->chn_status[vi_chn].size.height = g_vi_ctx->chn_attr[vi_chn].size.height;
			g_vi_ctx->chn_status[vi_chn].int_cnt = 0;
			g_vi_ctx->chn_status[vi_chn].frame_num = 0;
			g_vi_ctx->chn_status[vi_chn].prev_time = 0;
			g_vi_ctx->chn_status[vi_chn].frame_rate = 0;

			base_mod_jobs_init(&vi_jobs[vi_chn], 0, num_buffers, g_vi_ctx->chn_attr[vi_chn].depth);

			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
				u8 j = 0;

				for (j = 0; j < num_buffers; j++) {
					rc = vi_sdk_qbuf(chn, NULL);
					if (rc) {
						vi_pr(VI_ERR, "vi_qbuf error (%d)", rc);
						goto ERR_QBUF;
					}
				}
			}
		}
	}

	if (create_thread) {
		rc = vi_create_thread(gvdev, E_VI_TH_EVENT_HANDLER);
		if (rc) {
			vi_pr(VI_ERR, "Failed to create VI_EVENT_HANDLER thread\n");
			goto ERR_CREATE_THREAD;
		}
	}

	if (vi_start_streaming(gvdev)) {
		vi_pr(VI_ERR, "Failed to vi start streaming\n");
		rc = -EAGAIN;
		goto ERR_START_STREAMING;
	}

	atomic_set(&gvdev->isp_streamon, 1);

	return rc;

ERR_START_STREAMING:
	vi_destory_thread(gvdev, E_VI_TH_EVENT_HANDLER);
ERR_CREATE_THREAD:
ERR_QBUF:
	for (i = 0; i < g_vi_ctx->total_chn_num; i++) {
		mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = 0, .chn_id = i};

		base_mod_jobs_exit(&vi_jobs[chn.chn_id]);
	}

	return rc;
}

int vi_disable_chn(int vi_chn)
{
	int rc = 0, i = 0, j = 0;
	struct isp_ctx *ctx = &gvdev->ctx;

	for (i = 0; i < ctx->total_chn_num; i++) {
		vi_chn = i;

		g_vi_ctx->is_chn_enable[vi_chn] = 0;
		g_vi_ctx->chn_status[vi_chn].enable = 0;
		g_vi_ctx->chn_status[vi_chn].frame_num = 0;
		g_vi_ctx->chn_status[vi_chn].prev_time = 0;
		g_vi_ctx->chn_status[vi_chn].frame_rate = 0;

#if 0
		if (g_vi_mesh[vi_chn].paddr && g_vi_mesh[vi_chn].paddr != DEFAULT_MESH_PADDR)
			base_ion_free(g_vi_mesh[vi_chn].paddr);
#endif
		g_vi_mesh[vi_chn].paddr = 0;
		g_vi_mesh[vi_chn].vaddr = 0;

		if (vi_chn == (ctx->total_chn_num - 1)) {

			vi_destory_thread(gvdev, E_VI_TH_EVENT_HANDLER);
			vi_destory_dbg_thread(gvdev);

			if (vi_stop_streaming(gvdev)) {
				vi_pr(VI_ERR, "Failed to vi stop streaming\n");
				return -EAGAIN;
			}

			atomic_set(&gvdev->isp_streamon, 0);

			for (j = 0; j < ctx->total_chn_num; j++) {
				mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = 0, .chn_id = j};

				base_mod_jobs_exit(&vi_jobs[chn.chn_id]);
			}

			ctx->total_chn_num = 0;
			g_vi_ctx->total_chn_num = 0;
			g_vi_ctx->total_dev_num = 0;
		}
	}
	return rc;
}

int vi_sdk_enable_chn(int vi_pipe, int vi_chn)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_chn_enable[vi_chn] == 1) {
		vi_pr(VI_ERR, "vi chn(%d) is already enabled.\n", vi_chn);
		return ERR_VI_FAILED_NOT_DISABLED;
	}

	if (g_vi_ctx->total_dev_num != vi_chn + 1)
		return 0;

	ret = vi_get_ion_buf(gvdev);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI getIonBuf is failed\n");
		return ret;
	}

	ret = vi_enable_chn(0);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI enable chn is failed\n");
		return ret;
	}

	return 0;
}

int vi_sdk_disable_chn(int vi_pipe, int vi_chn)
{
	int ret = 0;
	u8  i = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->is_chn_enable[vi_chn] == 0) {
		vi_pr(VI_INFO, "vi chn(%d) is already disabled.", vi_chn);
		return 0;
	}

	g_vi_ctx->is_chn_enable[vi_chn] = 0;
	memset(&g_vi_ctx->chn_attr[vi_chn], 0, sizeof(vi_chn_attr_s));
	memset(&g_vi_ctx->chn_crop[vi_chn], 0, sizeof(vi_crop_info_s));

	for (i = 0; i < g_vi_ctx->total_chn_num; i++) {
		if (g_vi_ctx->is_chn_enable[i])
			return 0;
	}

	ret = vi_disable_chn(0);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI disable chn is failed\n");
		return ret;
	}

	ret = vi_free_ion_buf(gvdev);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI freeIonBuf is failed\n");
		return ERR_SYS_ILLEGAL_PARAM;
	}

	return 0;
}

int vi_get_chn_frame(int vi_pipe, int vi_chn, video_frame_info_s *frame_info, int millisec)
{
	vb_blk blk;
	struct vb_s *vb;
	int ret;
	mmf_chn_s chn = {.mod_id = ID_VI, .dev_id = vi_pipe, .chn_id = vi_chn};
	int i = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_chn);
	if (ret != 0)
		return ret;

	memset(frame_info, 0, sizeof(*frame_info));
	ret = base_get_chn_buffer(chn, &vi_jobs[chn.chn_id], &blk, millisec);
	if (ret != 0) {
		vi_pr(VI_ERR, "vi get pipe_%d, chn_%d buf fail\n", vi_pipe, vi_chn);
		return -1;
	}

	vb = (struct vb_s *)blk;
	if (vi_chn >= VI_EXT_CHN_START)
		frame_info->video_frame.pixel_format = g_vi_ctx->ext_chn_attr[vi_chn - VI_EXT_CHN_START].pixel_format;
	else
		frame_info->video_frame.pixel_format = g_vi_ctx->chn_attr[vi_chn].pixel_format;
	frame_info->video_frame.width = vb->buf.size.width;
	frame_info->video_frame.height = vb->buf.size.height;
	frame_info->video_frame.time_ref = vb->buf.frm_num;
	frame_info->video_frame.pts = vb->buf.pts;
	for (i = 0; i < 3; ++i) {
		frame_info->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		frame_info->video_frame.length[i] = vb->buf.length[i];
		frame_info->video_frame.stride[i] = vb->buf.stride[i];
	}

	frame_info->video_frame.offset_top = vb->buf.offset_top;
	frame_info->video_frame.offset_bottom = vb->buf.offset_bottom;
	frame_info->video_frame.offset_left = vb->buf.offset_left;
	frame_info->video_frame.offset_right = vb->buf.offset_right;
	frame_info->video_frame.private_data = vb;

	vi_pr(VI_DBG, "pixfmt(%d), w(%d), h(%d), pts(%lld), addr(0x%llx, 0x%llx, 0x%llx)\n",
			frame_info->video_frame.pixel_format, frame_info->video_frame.width,
			frame_info->video_frame.height, frame_info->video_frame.pts,
			frame_info->video_frame.phyaddr[0], frame_info->video_frame.phyaddr[1],
			frame_info->video_frame.phyaddr[2]);
	vi_pr(VI_DBG, "length(%d, %d, %d), stride(%d, %d, %d)\n",
			frame_info->video_frame.length[0], frame_info->video_frame.length[1],
			frame_info->video_frame.length[2], frame_info->video_frame.stride[0],
			frame_info->video_frame.stride[1], frame_info->video_frame.stride[2]);

	return ret;
}

int vi_release_chn_frame(int vi_pipe, int vi_chn, video_frame_info_s *frame_info)
{
	vb_blk blk;
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_chn);
	if (ret != 0)
		return ret;

	blk = vb_phys_addr2handle(frame_info->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		vi_pr(VI_ERR, "Invalid phy-address(%llx) in pvideo_frame. Can't find vb_blk.\n"
			    , frame_info->video_frame.phyaddr[0]);
		return -1;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	vi_pr(VI_DBG, "release chn frame, addr(0x%llx)\n",
			frame_info->video_frame.phyaddr[0]);

	return 0;
}

int vi_set_chn_crop(int vi_pipe, int vi_chn, vi_crop_info_s *chn_crop)
{
	struct isp_ctx *ctx = &gvdev->ctx;
	struct vi_rect crop;
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	if (!ctx->isp_pipe_cfg[vi_pipe].is_offline_scaler) { //online2sc
		vi_pr(VI_ERR, "not support online2sc");
		return ERR_VI_NOT_SUPPORT;
	}

	if ((chn_crop->crop_rect.x + chn_crop->crop_rect.width) >
		g_vi_ctx->pipe_attr[vi_pipe].max_width ||
		(chn_crop->crop_rect.y + chn_crop->crop_rect.height) >
		g_vi_ctx->pipe_attr[vi_pipe].max_height) {
		vi_pr(VI_ERR, "crop_x(%d)+w(%d) or y(%d)+h(%d) is bigger than chn_w(%d)_h(%d)\n",
					chn_crop->crop_rect.x,
					chn_crop->crop_rect.width,
					chn_crop->crop_rect.y,
					chn_crop->crop_rect.height,
					g_vi_ctx->pipe_attr[vi_pipe].max_width,
					g_vi_ctx->pipe_attr[vi_pipe].max_height);
		return ERR_VI_INVALID_PARA;
	}

	crop.x = chn_crop->crop_rect.x;
	crop.y = chn_crop->crop_rect.y;
	crop.w = chn_crop->crop_rect.width;
	crop.h = chn_crop->crop_rect.height;

	vi_pr(VI_INFO, "set chn crop x(%d), y(%d), w(%d), h(%d)\n", crop.x, crop.y, crop.w, crop.h);

	//for tile mode crop, don't update crop now;
	if (!ctx->isp_pipe_cfg[vi_pipe].is_tile) {
		ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
		crop.x >>= 1;
		crop.y >>= 1;
		crop.w >>= 1;
		crop.h >>= 1;
		ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);
	}

	g_vi_ctx->chn_attr[vi_chn].size.width	= chn_crop->crop_rect.width;
	g_vi_ctx->chn_attr[vi_chn].size.height	= chn_crop->crop_rect.height;
	g_vi_ctx->chn_crop[vi_chn]			= *chn_crop;

	return 0;
}

int vi_get_chn_crop(int vi_pipe, int vi_chn, vi_crop_info_s *chn_crop)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	*chn_crop = g_vi_ctx->chn_crop[vi_chn];

	return ret;
}

//TODO need refactor
int vi_set_pipe_crop(int vi_pipe, crop_info_s *crop_info)
{
	int ret = 0;
	struct isp_ctx *ctx = &gvdev->ctx;
	struct vi_rect crop;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	if (crop_info->rect.x % 2 || crop_info->rect.y % 2 ||
	    crop_info->rect.width % 2 || crop_info->rect.height % 2) {
		vi_pr(VI_ERR, "crop_x(%d)_y(%d)_w(%d)_h(%d) must be multiple of 2.\n",
		      crop_info->rect.x, crop_info->rect.y,
		      crop_info->rect.width, crop_info->rect.height);
		return ERR_VI_INVALID_PARA;
	}

	if (crop_info->rect.x < 0 || crop_info->rect.y < 0 ||
	    crop_info->rect.x + crop_info->rect.width > g_vi_ctx->pipe_attr[vi_pipe].max_width ||
	    crop_info->rect.y + crop_info->rect.height > g_vi_ctx->pipe_attr[vi_pipe].max_height) {
		vi_pr(VI_ERR, "crop_x(%d)_y(%d) is invalid.\n",
					crop_info->rect.x,
					crop_info->rect.y);
		return ERR_VI_INVALID_PARA;
	}

	g_vi_ctx->pipe_crop[vi_pipe] = *crop_info;

	crop.x = crop_info->rect.x;
	crop.y = crop_info->rect.y;
	crop.w = crop_info->rect.width;
	crop.h = crop_info->rect.height;

	vi_pr(VI_INFO, "set chn crop x(%d), y(%d), w(%d), h(%d)\n", crop.x, crop.y, crop.w, crop.h);

	ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_Y, crop);
	crop.w >>= 1;
	crop.h >>= 1;
	ispblk_crop_config(ctx, ISP_BLK_ID_YUV_CROP_C, crop);

	g_vi_ctx->chn_attr[0].size.width	= crop_info->rect.width;
	g_vi_ctx->chn_attr[0].size.height	= crop_info->rect.height;
	g_vi_ctx->chn_crop[0].crop_rect		= crop_info->rect;

	return 0;
}

int vi_get_pipe_crop(int vi_pipe, crop_info_s *crop_info)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	*crop_info = g_vi_ctx->pipe_crop[vi_pipe];

	return 0;
}

int vi_set_pipe_attr(int vi_pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	g_vi_ctx->pipe_attr[vi_pipe] = *pipe_attr;

	return 0;
}

int vi_get_pipe_attr(int vi_pipe, vi_pipe_attr_s *pipe_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	if (g_vi_ctx->pipe_attr[vi_pipe].max_width == 0 &&
	    g_vi_ctx->pipe_attr[vi_pipe].max_height == 0) {
		vi_pr(VI_ERR, "Setpipe_attr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	*pipe_attr = g_vi_ctx->pipe_attr[vi_pipe];

	return 0;
}

int vi_set_pipe_dump_attr(int vi_pipe, vi_dump_attr_s *dump_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	g_vi_ctx->dump_attr[vi_pipe] = *dump_attr;

	return 0;
}

int vi_get_pipe_dump_attr(int vi_pipe, vi_dump_attr_s *dump_attr)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	*dump_attr = g_vi_ctx->dump_attr[vi_pipe];

	return 0;
}

int vi_get_pipe_frame(int vi_pipe, video_frame_info_s *frame_info, int millisec)
{
	int ret;
	struct sop_vip_isp_raw_blk dump[2];
	u32 dev_frm_w, dev_frm_h, frm_w, frm_h, raw_num;
	u64 phyaddr = 0, addr = 0x00;
	vb_blk  tmp_vb = VB_INVALID_HANDLE;
	int blk_size;
	int dev_id = 0, frm_num = 1, i = 0;
	struct vi_rect rawdump_crop;
	struct isp_ctx *ctx = &gvdev->ctx;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	if (atomic_read(&gvdev->isp_streamoff) == 1) {
		vi_pr(VI_ERR, "StartStream first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	if (g_vi_ctx->dump_attr[vi_pipe].enable == 0) {
		vi_pr(VI_ERR, "SetPipeDumpAttr first\n");
		return ERR_VI_FAILED_NOTCONFIG;
	}

	if (g_vi_ctx->dump_attr[vi_pipe].dump_type == VI_DUMP_TYPE_YUV ||
	    g_vi_ctx->dump_attr[vi_pipe].dump_type == VI_DUMP_TYPE_IR) {
		vi_pr(VI_ERR, "IR or yuv raw dump is not supported.\n");
		return ERR_VI_FAILED_NOT_ENABLED;
	}

	memset(dump, 0, sizeof(dump));
	raw_num = vi_get_raw_num_by_dev(ctx, vi_pipe);
	dump[0].raw_dump.raw_num = raw_num;

	dev_frm_w = g_vi_ctx->dev_attr[vi_pipe].size.width;
	dev_frm_h = g_vi_ctx->dev_attr[vi_pipe].size.height;

	memset(&rawdump_crop, 0, sizeof(rawdump_crop));
	if ((frame_info[0].video_frame.offset_top != 0) ||
	     frame_info[0].video_frame.offset_bottom != 0 ||
	     frame_info[0].video_frame.offset_left != 0 ||
	     frame_info[0].video_frame.offset_right != 0) {
		rawdump_crop.x = frame_info[0].video_frame.offset_left;
		rawdump_crop.y = frame_info[0].video_frame.offset_top;
		rawdump_crop.w = dev_frm_w - rawdump_crop.x - frame_info[0].video_frame.offset_right;
		rawdump_crop.h = dev_frm_h - rawdump_crop.y - frame_info[0].video_frame.offset_bottom;

		vi_pr(VI_INFO, "set rawdump crop x(%d), y(%d), w(%d), h(%d)\n",
			rawdump_crop.x, rawdump_crop.y, rawdump_crop.w, rawdump_crop.h);

		frm_w = rawdump_crop.w;
		frm_h = rawdump_crop.h;
	} else {
		frm_w = dev_frm_w;
		frm_h = dev_frm_h;
	}
	ctx->isp_pipe_cfg[raw_num].rawdump_crop = rawdump_crop;
	ctx->isp_pipe_cfg[raw_num].rawdump_crop_se = rawdump_crop;

	g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;

	blk_size = vi_getrawbuffersize(frm_w, frm_h, PIXEL_FORMAT_RGB_BAYER_12BPP,
				       g_vi_ctx->pipe_attr[0].compress_mode, 16, g_vi_ctx->is_tile);
	vi_pr(VI_INFO, "frm_w(%d), frm_h(%d), blk_size: %d\n", frm_w, frm_h, blk_size);

	/* Check if it is valid, such as the frame from VPSS. */
	if (!frame_info[0].video_frame.phyaddr[0]) {
		/* If it is valid, we can use its VB to receive raw frames. */
		tmp_vb = vb_phys_addr2handle(frame_info[0].video_frame.phyaddr[0]);
	}

	if (tmp_vb == VB_INVALID_HANDLE) {

		g_vi_ctx->vi_raw_blk[0] = vb_get_block_with_id(VB_INVALID_POOLID, blk_size, ID_VI);
		if (g_vi_ctx->vi_raw_blk[0] == VB_INVALID_HANDLE) {
			vi_pr(VI_ERR, "Alloc VB blk for RAW_LE dump failed\n");
			return -1;
		}

		phyaddr = vb_handle2phys_addr(g_vi_ctx->vi_raw_blk[0]);

	} else {

		if (blk_size > frame_info[0].video_frame.length[0]) {
			vi_pr(VI_ERR, "input vb blk too small, in: %d, rq: %d\n",
				blk_size, frame_info[0].video_frame.length[0]);
			return -1;
		}

		phyaddr = frame_info[0].video_frame.phyaddr[0];
		g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	}

	dump[0].raw_dump.phy_addr = phyaddr;

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		frm_num = 2;
		tmp_vb = VB_INVALID_HANDLE;

		if (frame_info[1].video_frame.phyaddr[0] != 0) {
			tmp_vb = vb_phys_addr2handle(frame_info[1].video_frame.phyaddr[0]);

			if (tmp_vb == VB_INVALID_HANDLE) {
				addr = frame_info[0].video_frame.phyaddr[0] +
					frame_info[0].video_frame.length[0];
			}

			/* You can use the second half of the same VB to receive SE raw frame. */
			if (addr == frame_info[1].video_frame.phyaddr[0]) {
				tmp_vb = 0;
			}
		}

		if (tmp_vb == VB_INVALID_HANDLE) {

			g_vi_ctx->vi_raw_blk[1] = vb_get_block_with_id(VB_INVALID_POOLID, blk_size, ID_VI);
			if (g_vi_ctx->vi_raw_blk[1] == VB_INVALID_HANDLE) {
				vb_release_block(g_vi_ctx->vi_raw_blk[0]);
				g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
				vi_pr(VI_ERR, "Alloc VB blk for RAW_SE dump failed\n");
				return -1;
			}

			phyaddr = vb_handle2phys_addr(g_vi_ctx->vi_raw_blk[1]);

		} else {

			if (blk_size > frame_info[1].video_frame.length[0]) {
				vi_pr(VI_ERR, "input vb blk too small, in: %d, rq: %d\n",
					blk_size, frame_info[1].video_frame.length[0]);
				return -1;
			}

			phyaddr = frame_info[1].video_frame.phyaddr[0];
			g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		dump[1].raw_dump.phy_addr = phyaddr;
	}

	if (millisec >= 0)
		dump[0].time_out = dump[1].time_out = millisec;

	ret = isp_raw_dump(gvdev, &dump[0]);
	if (ret != 0) {
		vi_pr(VI_ERR, "_isp_raw_dump fail\n");
		return -1;
	}

	if (dump[0].is_b_not_rls) {
		if (g_vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[0]);
			g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}
		if (g_vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[1]);
			g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Release pipe frame first, buffer not release.\n");
		return -1;
	}

	if (dump[0].is_timeout) {
		if (g_vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[0]);
			g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}

		if (g_vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[1]);
			g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Get pipe frame time out(%d)\n", millisec);
		return -1;
	}

	if (dump[0].is_sig_int) {
		if (g_vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[0]);
			g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
		}

		if (g_vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
			vb_release_block(g_vi_ctx->vi_raw_blk[1]);
			g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
		}

		vi_pr(VI_ERR, "Get pipe frame signal interrupt\n");
		return -1;
	}


	for (; i < frm_num; i++) {
		frame_info[i].video_frame.phyaddr[0]  = dump[i].raw_dump.phy_addr;
		frame_info[i].video_frame.length[0]   = blk_size;
		frame_info[i].video_frame.bayer_format  = g_vi_ctx->dev_attr[dev_id].bayer_format;
		frame_info[i].video_frame.compress_mode = g_vi_ctx->pipe_attr[0].compress_mode;
		frame_info[i].video_frame.width       = dump[i].src_w;
		frame_info[i].video_frame.height      = dump[i].src_h;
		frame_info[i].video_frame.offset_left  = dump[i].crop_x;
		frame_info[i].video_frame.offset_top   = dump[i].crop_y;
		frame_info[i].video_frame.time_ref     = dump[i].frm_num;
	}

	return 0;
}

int vi_release_pipe_frame(int vi_pipe, video_frame_info_s *frame_info)
{
	u8 i = 0;
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	for (i = 0; i < ISP_PRERAW_MAX; i++) {
		free_isp_byr(i);
	}

	if (g_vi_ctx->vi_raw_blk[0] != VB_INVALID_HANDLE) {
		vb_release_block(g_vi_ctx->vi_raw_blk[0]);
		g_vi_ctx->vi_raw_blk[0] = VB_INVALID_HANDLE;
	}

	if (g_vi_ctx->vi_raw_blk[1] != VB_INVALID_HANDLE) {
		vb_release_block(g_vi_ctx->vi_raw_blk[1]);
		g_vi_ctx->vi_raw_blk[1] = VB_INVALID_HANDLE;
	}

	return 0;
}

int vi_get_smooth_rawdump(int vi_pipe, video_frame_info_s *frame_info, int millisec)
{
	int ret;
	struct isp_ctx *ctx = &gvdev->ctx;
	struct sop_vip_isp_raw_blk dump[2];
	int dev_id = 0, frm_num = 1, i = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	memset(dump, 0, sizeof(dump));
	dump[0].raw_dump.raw_num = dump[1].raw_dump.raw_num = vi_get_raw_num_by_dev(ctx, vi_pipe);
	dump[0].time_out = dump[1].time_out = millisec;

	ret = isp_get_smooth_raw_dump(gvdev, dump);
	if (ret != 0) {
		vi_pr(VI_ERR, "isp_smooth_raw_dump failed\n");
		return -1;
	}

	if (dump[0].is_b_not_rls) {
		vi_pr(VI_ERR, "Release pipe frame first, buffer not release.\n");
		return -1;
	}

	if (dump[0].is_timeout) {
		vi_pr(VI_ERR, "Get pipe frame time out(%d)\n", millisec);
		return -1;
	}

	if (dump[0].is_sig_int) {
		vi_pr(VI_ERR, "Get pipe frame signal interrupt\n");
		return -1;
	}

	if (g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_LINE ||
	    g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME ||
	    g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME_FULL_RATE)
		frm_num = 2;
	else
		frm_num = 1;

	for (; i < frm_num; i++) {
		frame_info[i].video_frame.phyaddr[0]  = dump[i].raw_dump.phy_addr;
		frame_info[i].video_frame.length[0]   = dump[i].raw_dump.size;
		frame_info[i].video_frame.bayer_format  = g_vi_ctx->dev_attr[dev_id].bayer_format;
		frame_info[i].video_frame.compress_mode = g_vi_ctx->pipe_attr[0].compress_mode;
		frame_info[i].video_frame.width       = dump[i].src_w;
		frame_info[i].video_frame.height      = dump[i].src_h;
		frame_info[i].video_frame.offset_left  = dump[i].crop_x;
		frame_info[i].video_frame.offset_top   = dump[i].crop_y;
		frame_info[i].video_frame.time_ref     = dump[i].frm_num;
		vi_pr(VI_DBG, "Get paddr(0x%llx) size(%d) frm_num(%d)\n",
			frame_info[i].video_frame.phyaddr[0],
			frame_info[i].video_frame.length[0],
			frame_info[i].video_frame.time_ref);
	}

	return 0;
}

int vi_put_smooth_rawdump(int vi_pipe, video_frame_info_s *frame_info)
{
	int ret;
	struct isp_ctx *ctx = &gvdev->ctx;
	struct sop_vip_isp_raw_blk dump[2];
	vb_blk vb;
	int frm_num, i, dev_num;

	memset(dump, 0, sizeof(dump));

	if (g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_LINE ||
	    g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME ||
	    g_vi_ctx->dev_attr[vi_pipe].wdr_attr.wdr_mode == WDR_MODE_2TO1_FRAME_FULL_RATE)
		frm_num = 2;
	else
		frm_num = 1;

	for (i = 0; i < frm_num; i++) {
		vb = vb_phys_addr2handle(frame_info[i].video_frame.phyaddr[0]);
		if (vb == VB_INVALID_HANDLE) {
			vi_pr(VI_ERR, "Can't get valid vb_blk.\n");
			return -1;
		}

		dump[i].raw_dump.phy_addr = frame_info[i].video_frame.phyaddr[0];
		vi_pr(VI_DBG, "Put paddr(0x%llx)\n", dump[i].raw_dump.phy_addr);

		dev_num = dump[i].raw_dump.raw_num;
		dump[i].raw_dump.raw_num = vi_get_raw_num_by_dev(ctx, dev_num);
	}

	ret = isp_put_smooth_raw_dump(gvdev, dump);
	if (ret != 0) {
		vi_pr(VI_ERR, "isp_put_smooth_raw_dump failed\n");
		return -1;
	}

	return 0;
}

static int _vi_update_rotation_mesh(int vi_chn, rotation_e rotation)
{
	struct gdc_mesh *pmesh = &g_vi_mesh[vi_chn];

	mutex_lock(&pmesh->lock);
	pmesh->paddr = DEFAULT_MESH_PADDR;
	g_vi_ctx->rotation[vi_chn] = rotation;
	mutex_unlock(&pmesh->lock);
	return 0;
}

static int _vi_update_ldc_mesh(vpss_chn vi_chn, const vi_ldc_attr_s *ldc_attr, u64 paddr)
{
	u64 paddr_old;
	struct gdc_mesh *pmesh = &g_vi_mesh[vi_chn];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;

	g_vi_ctx->ldc_attr[vi_chn] = *ldc_attr;
	mutex_unlock(&pmesh->lock);

	vi_pr(VI_DBG, "Chn(%d) mesh base(0x%llx)\n", vi_chn, (unsigned long long)paddr);
	vi_pr(VI_DBG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d, rotation=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.x_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio,
			ldc_attr->attr.rotation);
	return 0;
}

int vi_set_chn_rotation(int vi_pipe, int vi_chn, rotation_e rotation)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	vi_pr(VI_DBG, "Chn(%d) rotation(%d).\n", vi_chn, rotation);

	return _vi_update_rotation_mesh(vi_chn, rotation);
}

int vi_get_chn_rotation(int vi_pipe, int vi_chn, rotation_e *rotation)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_pipe_created(vi_pipe);
	if (ret != 0)
		return ret;

	*rotation = g_vi_ctx->rotation[vi_chn];

	return ret;
}

int vi_set_chn_ldc_attr(int vi_chn, const vi_ldc_attr_s *ldc_attr, u64 mesh_addr)
{
	vi_pr(VI_DBG, "Chn(%d) mesh base(0x%llx)\n", vi_chn, (unsigned long long)mesh_addr);
	return _vi_update_ldc_mesh(vi_chn, ldc_attr, mesh_addr);
}

int vi_get_chn_ldc_attr(int vi_pipe, int vi_chn, struct vi_chn_ldc_cfg *cfg)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	cfg->vi_chn = vi_chn;
	cfg->ldc_attr = g_vi_ctx->ldc_attr[vi_chn];
	cfg->mesh_handle = g_vi_mesh[vi_chn].paddr;

	return ret;
}

int vi_set_chn_flip_mirror(int vi_pipe, int vi_chn, struct vi_chn_flip_mirror_cfg *cfg)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_chn);
	if (ret != 0)
		return ret;

	g_vi_ctx->chn_attr[vi_chn].flip = cfg->flip;
	g_vi_ctx->chn_attr[vi_chn].mirror = cfg->mirror;

	return ret;
}

int vi_get_chn_flip_mirror(int vi_pipe, int vi_chn, struct vi_chn_flip_mirror_cfg *cfg)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_enable(vi_chn);
	if (ret != 0)
		return ret;

	cfg->flip = g_vi_ctx->chn_attr[vi_chn].flip;
	cfg->mirror = g_vi_ctx->chn_attr[vi_chn].mirror;

	return ret;
}

int vi_attach_vb_pool(int vi_pipe, int vi_chn, vb_pool vbp)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	g_vi_ctx->chn_attr[vi_chn].bind_vb_pool = vbp;

	vi_pr(VI_DBG, "Chn(%d) attach VbPool(%d)\n", vi_chn, g_vi_ctx->chn_attr[vi_chn].bind_vb_pool);

	return 0;
}

int vi_detach_vb_pool(int vi_pipe, int vi_chn)
{
	int ret = 0;

	ret = check_vi_pipe_valid(vi_pipe);
	if (ret != 0)
		return ret;

	ret = check_vi_chn_valid(vi_chn);
	if (ret != 0)
		return ret;

	g_vi_ctx->chn_attr[vi_chn].bind_vb_pool = VB_INVALID_POOLID;

	vi_pr(VI_DBG, "Chn(%d) detach VbPool\n", vi_chn);

	return 0;
}

/*****************************************************************************
 *  SDK layer ioctl operations for vi.c
 ****************************************************************************/
long vi_sdk_ctrl(struct sop_vi_dev *vdev, struct vi_ext_control *p)
{
	u32 id = p->sdk_id;
	long rc = -EINVAL;
	gvdev = vdev;

	switch (id) {
	case VI_SDK_SET_DEV_ATTR:
	{
		vi_dev_attr_s dev_attr;

		if (copy_from_user(&dev_attr, p->sdk_cfg.ptr, sizeof(vi_dev_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_set_dev_attr(p->sdk_cfg.dev, &dev_attr);
		break;
	}
	case VI_SDK_GET_DEV_ATTR:
	{
		vi_dev_attr_s dev_attr;

		rc = vi_get_dev_attr(p->sdk_cfg.dev, &dev_attr);

		if (copy_to_user(p->sdk_cfg.ptr, &dev_attr, sizeof(vi_dev_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_attr_s copy to user fail.\n");
			rc = -1;
			break;
		}

		rc = 0;
		break;
	}
	case VI_SDK_SET_DEV_BIND_ATTR:
	{
		vi_dev_bind_pipe_s dev_bind_attr;

		if (copy_from_user(&dev_bind_attr, p->sdk_cfg.ptr, sizeof(vi_dev_bind_pipe_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_bind_pipe_s copy from user fail.\n");
			break;
		}

		rc = vi_set_dev_bind_attr(p->sdk_cfg.dev, &dev_bind_attr);
		break;
	}
	case VI_SDK_GET_DEV_BIND_ATTR:
	{
		vi_dev_bind_pipe_s dev_bind_attr;

		rc = vi_get_dev_bind_attr(p->sdk_cfg.dev, &dev_bind_attr);

		if (copy_to_user(p->sdk_cfg.ptr, &dev_bind_attr, sizeof(vi_dev_bind_pipe_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_bind_pipe_s copy to user fail.\n");
			rc = -1;
			break;
		}

		rc = 0;
		break;
	}
	case VI_SDK_SET_DEV_UNBIND_ATTR:
	{
		rc = vi_set_dev_unbind_attr(p->sdk_cfg.dev);
		break;
	}
	case VI_SDK_ENABLE_DEV:
	{
		rc = vi_enable_dev(p->sdk_cfg.dev);
		break;
	}
	case VI_SDK_DISABLE_DEV:
	{
		rc = vi_disable_dev(p->sdk_cfg.dev);
		break;
	}
	case VI_SDK_CREATE_PIPE:
	{
		vi_pipe_attr_s pipe_attr;

		if (copy_from_user(&pipe_attr, p->sdk_cfg.ptr, sizeof(vi_pipe_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_create_pipe(p->sdk_cfg.pipe, &pipe_attr);
		break;
	}
	case VI_SDK_DESTROY_PIPE:
	{
		rc = vi_destroy_pipe(p->sdk_cfg.pipe);
		break;
	}
	case VI_SDK_SET_PIPE_ATTR:
	{
		vi_pipe_attr_s pipe_attr;

		if (copy_from_user(&pipe_attr, p->sdk_cfg.ptr, sizeof(vi_pipe_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_set_pipe_attr(p->sdk_cfg.pipe, &pipe_attr);

		break;
	}
	case VI_SDK_GET_PIPE_ATTR:
	{
		vi_pipe_attr_s pipe_attr;

		memset(&pipe_attr, 0, sizeof(pipe_attr));

		rc = vi_get_pipe_attr(p->sdk_cfg.pipe, &pipe_attr);

		if (copy_to_user(p->sdk_cfg.ptr, &pipe_attr, sizeof(vi_pipe_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_attr_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_START_PIPE:
	{
		rc = vi_start_pipe(p->sdk_cfg.pipe);
		break;
	}
	case VI_SDK_STOP_PIPE:
	{
		rc = 0;
		break;
	}
	case VI_SDK_SET_CHN_ATTR:
	{
		vi_chn_attr_s chn_attr;

		if (copy_from_user(&chn_attr, p->sdk_cfg.ptr, sizeof(vi_chn_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_chn_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_set_chn_attr(p->sdk_cfg.pipe, p->sdk_cfg.chn, &chn_attr);
		break;
	}
	case VI_SDK_GET_CHN_ATTR:
	{
		vi_chn_attr_s chn_attr;

		rc = vi_get_chn_attr(p->sdk_cfg.pipe, p->sdk_cfg.chn, &chn_attr);
		if (copy_to_user(p->sdk_cfg.ptr, &chn_attr, sizeof(vi_chn_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_attr_s copy to user fail.\n");
			rc = -1;
			break;
		}

		rc = 0;
		break;
	}
	case VI_SDK_ENABLE_CHN:
	{
		rc = vi_sdk_enable_chn(p->sdk_cfg.pipe, p->sdk_cfg.chn);
		break;
	}
	case VI_SDK_DISABLE_CHN:
	{
		rc = vi_sdk_disable_chn(p->sdk_cfg.pipe, p->sdk_cfg.chn);
		break;
	}
	case VI_SDK_SET_MOTION_LV:
	{
		struct mlv_info_s mlv_i;

		if (copy_from_user(&mlv_i, p->sdk_cfg.ptr, sizeof(struct mlv_info_s)) != 0) {
			vi_pr(VI_ERR, "struct mlv_info copy from user fail.\n");
			break;
		}

		rc = vi_set_motion_lv(mlv_i);
		break;
	}
	case VI_SDK_ENABLE_DIS:
	{
		g_vi_ctx->is_dis_enable[p->sdk_cfg.pipe] = 1;

		vi_pr(VI_DBG, "pipe_%d enable dis\n", p->sdk_cfg.pipe);

		rc = 0;
		break;
	}
	case VI_SDK_DISABLE_DIS:
	{
		g_vi_ctx->is_dis_enable[p->sdk_cfg.pipe] = 0;

		if (wq_has_sleeper(&dis_wait_q[p->sdk_cfg.pipe])) {
			dis_flag[p->sdk_cfg.pipe] = 1;
			wake_up(&dis_wait_q[p->sdk_cfg.pipe]);
		}

		vi_pr(VI_DBG, "pipe_%d disable dis\n", p->sdk_cfg.pipe);

		rc = 0;
		break;
	}
	case VI_SDK_SET_DIS_INFO:
	{
		struct dis_info_s dis_i;

		if (copy_from_user(&dis_i, p->sdk_cfg.ptr, sizeof(struct dis_info_s)) != 0) {
			vi_pr(VI_ERR, "struct dis_info_s copy from user fail.\n");
			break;
		}

		rc = vi_set_dis_info(dis_i);
		break;
	}
	case VI_SDK_SET_BYPASS_FRM:
	{
		rc = vi_set_bypass_frm(p->sdk_cfg.pipe, p->sdk_cfg.val);
		break;
	}
	case VI_SDK_SET_PIPE_FRM_SRC:
	{
		vi_pipe_frame_source_e src;

		if (copy_from_user(&src, p->sdk_cfg.ptr, sizeof(vi_pipe_frame_source_e)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_frame_source_e copy from user fail.\n");
			break;
		}

		rc = vi_set_pipe_frame_source(p->sdk_cfg.pipe, src);
		break;
	}
	case VI_SDK_GET_PIPE_FRM_SRC:
	{
		vi_pipe_frame_source_e src;

		rc = vi_get_pipe_frame_source(p->sdk_cfg.pipe, &src);

		if (copy_to_user(p->sdk_cfg.ptr, &src, sizeof(vi_pipe_frame_source_e)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_frame_source_e copy to user fail.\n");
			break;
		}
		break;
	}
	case VI_SDK_SEND_PIPE_RAW:
	{
		video_frame_info_s v_frm_info;

		if (copy_from_user(&v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s)) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_send_pipe_raw(p->sdk_cfg.pipe, &v_frm_info);
		break;
	}
	case VI_SDK_SET_DEV_TIMING_ATTR:
	{
		vi_dev_timing_attr_s dev_timing_attr;

		if (copy_from_user(&dev_timing_attr, p->sdk_cfg.ptr, sizeof(vi_dev_timing_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_timing_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_set_dev_timing_attr(p->sdk_cfg.dev, &dev_timing_attr);
		break;
	}
	case VI_SDK_GET_DEV_TIMING_ATTR:
	{
		vi_dev_timing_attr_s dev_timing_attr;

		rc = vi_get_dev_timing_attr(p->sdk_cfg.dev, &dev_timing_attr);

		if (copy_to_user(p->sdk_cfg.ptr, &dev_timing_attr, sizeof(vi_dev_timing_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_dev_timing_attr_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_GET_PIPE_STATUS:
	{
		vi_pipe_status_s status;

		rc = vi_get_pipe_status(p->sdk_cfg.pipe, &status);

		if (copy_to_user(p->sdk_cfg.ptr, &status, sizeof(vi_pipe_status_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_status_s copy to user fail.\n");
			rc = -1;
			break;
		}

		break;
	}
	case VI_SDK_GET_DEV_STATUS:
	{
		u8 status = g_vi_ctx->is_dev_enable[p->sdk_cfg.dev];

		if (copy_to_user(p->sdk_cfg.ptr, &status, sizeof(status)) != 0) {
			vi_pr(VI_ERR, "VI_SDK_GET_DEV_STATUS copy to user fail.\n");
			rc = -1;
			break;
		}

		rc = 0;

		break;
	}
	case VI_SDK_GET_CHN_STATUS:
	{
		vi_chn_status_s status;

		rc = vi_get_chn_status(p->sdk_cfg.chn, &status);

		if (copy_to_user(p->sdk_cfg.ptr, &status, sizeof(vi_chn_status_s)) != 0) {
			vi_pr(VI_ERR, "vi_chn_status_s copy to user fail.\n");
			rc = -1;
			break;
		}

		break;
	}
	case VI_SDK_GET_CHN_FRAME:
	{
		video_frame_info_s v_frm_info;

		if (copy_from_user(&v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s)) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_get_chn_frame(p->sdk_cfg.pipe, p->sdk_cfg.chn, &v_frm_info, p->sdk_cfg.val);

		if (copy_to_user(p->sdk_cfg.ptr, &v_frm_info, sizeof(video_frame_info_s)) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_RELEASE_CHN_FRAME:
	{
		video_frame_info_s v_frm_info;

		if (copy_from_user(&v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s)) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_release_chn_frame(p->sdk_cfg.pipe, p->sdk_cfg.chn, &v_frm_info);
		break;
	}
	case VI_SDK_SET_CHN_CROP:
	{
		vi_crop_info_s chn_crop;

		if (copy_from_user(&chn_crop, p->sdk_cfg.ptr, sizeof(vi_crop_info_s)) != 0) {
			vi_pr(VI_ERR, "vi_crop_info_s copy from user fail.\n");
			break;
		}

		rc = vi_set_chn_crop(p->sdk_cfg.pipe, p->sdk_cfg.chn, &chn_crop);
		break;
	}
	case VI_SDK_GET_CHN_CROP:
	{
		vi_crop_info_s chn_crop;

		memset(&chn_crop, 0, sizeof(chn_crop));

		rc = vi_get_chn_crop(p->sdk_cfg.pipe, p->sdk_cfg.chn, &chn_crop);

		if (copy_to_user(p->sdk_cfg.ptr, &chn_crop, sizeof(vi_crop_info_s)) != 0) {
			vi_pr(VI_ERR, "vi_crop_info_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_SET_PIPE_CROP:
	{
		crop_info_s crop_info;

		if (copy_from_user(&crop_info, p->sdk_cfg.ptr, sizeof(crop_info_s)) != 0) {
			vi_pr(VI_ERR, "crop_info_s copy from user fail.\n");
			break;
		}

		rc = vi_set_pipe_crop(p->sdk_cfg.pipe, &crop_info);
		break;
	}
	case VI_SDK_GET_PIPE_CROP:
	{
		crop_info_s crop_info;

		memset(&crop_info, 0, sizeof(crop_info_s));

		rc = vi_get_pipe_crop(p->sdk_cfg.pipe, &crop_info);

		if (copy_to_user(p->sdk_cfg.ptr, &crop_info, sizeof(crop_info_s)) != 0) {
			vi_pr(VI_ERR, "crop_info_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_GET_PIPE_FRAME:
	{
		video_frame_info_s v_frm_info[2];

		if (copy_from_user(v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_get_pipe_frame(p->sdk_cfg.pipe, v_frm_info, p->sdk_cfg.val);

		if (copy_to_user(p->sdk_cfg.ptr, v_frm_info, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_RELEASE_PIPE_FRAME:
	{
		video_frame_info_s v_frm_info[2];

		if (copy_from_user(v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_release_pipe_frame(p->sdk_cfg.pipe, v_frm_info);
		break;
	}
	case VI_SDK_START_SMOOTH_RAWDUMP:
	{
		struct sop_vip_isp_smooth_raw_param param;
		struct sop_vip_isp_raw_blk *raw_blk;
		u32 size;

		if (copy_from_user(&param, p->sdk_cfg.ptr, sizeof(struct sop_vip_isp_smooth_raw_param)) != 0) {
			vi_pr(VI_ERR, "sop_vip_isp_smooth_raw_param copy from user fail.\n");
			break;
		}

		size = sizeof(struct sop_vip_isp_raw_blk) * param.frm_num;
		raw_blk = kmalloc(size, GFP_KERNEL);
		if (raw_blk == NULL) {
			vi_pr(VI_ERR, "kmalloc failed need size(0x%x).\n", size);
			rc = -ENOMEM;
			break;
		}

		if (copy_from_user(raw_blk, (void __user *)param.raw_blk, size)) {
			vi_pr(VI_ERR, "sop_vip_isp_raw_blk copy from user fail.\n");
			kfree(raw_blk);
			break;
		}

		param.raw_num = vi_get_raw_num_by_dev(&vdev->ctx, param.raw_num);
		param.raw_blk = raw_blk;
		rc = isp_start_smooth_raw_dump(vdev, &param);

		kfree(raw_blk);
		break;
	}
	case VI_SDK_STOP_SMOOTH_RAWDUMP:
	{
		struct sop_vip_isp_smooth_raw_param param;

		if (copy_from_user(&param, p->sdk_cfg.ptr, sizeof(struct sop_vip_isp_smooth_raw_param)) != 0)
			break;

		param.raw_num = vi_get_raw_num_by_dev(&vdev->ctx, param.raw_num);
		rc = isp_stop_smooth_raw_dump(vdev, &param);
		break;
	}
	case VI_SDK_GET_SMOOTH_RAWDUMP:
	{
		video_frame_info_s v_frm_info[2];

		if (copy_from_user(v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_get_smooth_rawdump(p->sdk_cfg.pipe, v_frm_info, p->sdk_cfg.val);

		if (copy_to_user(p->sdk_cfg.ptr, v_frm_info, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_PUT_SMOOTH_RAWDUMP:
	{
		video_frame_info_s v_frm_info[2];

		if (copy_from_user(v_frm_info, p->sdk_cfg.ptr, sizeof(video_frame_info_s) * 2) != 0) {
			vi_pr(VI_ERR, "video_frame_info_s copy from user fail.\n");
			break;
		}

		rc = vi_put_smooth_rawdump(p->sdk_cfg.pipe, v_frm_info);
		break;
	}
	case VI_SDK_SET_CHN_ROTATION:
	{
		struct vi_chn_rot_cfg cfg;

		if (copy_from_user(&cfg, p->sdk_cfg.ptr, sizeof(cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_rot_cfg copy from user fail.\n");
			break;
		}

		rc = vi_set_chn_rotation(p->sdk_cfg.pipe, p->sdk_cfg.chn, cfg.rotation);
		break;
	}
	case VI_SDK_GET_CHN_ROTATION:
	{
		struct vi_chn_rot_cfg cfg;

		rc = vi_get_chn_rotation(p->sdk_cfg.pipe, p->sdk_cfg.chn, &cfg.rotation);

		if (copy_to_user(p->sdk_cfg.ptr, &cfg, sizeof(struct vi_chn_rot_cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_rot_cfg copy to user fail.\n");
			rc = -1;
			break;
		}

		break;
	}
	case VI_SDK_SET_CHN_LDC:
	{
		struct vi_chn_ldc_cfg cfg;
		int vi_chn;
		u64 mesh_addr;

		const vi_ldc_attr_s *ldc_attr = NULL;

		if (copy_from_user(&cfg, p->sdk_cfg.ptr, sizeof(cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_ldc_cfg copy from user fail.\n");
			break;
		}

		vi_chn = cfg.vi_chn;
		mesh_addr = cfg.mesh_handle;
		ldc_attr = &cfg.ldc_attr;

		rc = vi_set_chn_ldc_attr(vi_chn, ldc_attr, mesh_addr);
		break;
	}
	case VI_SDK_GET_CHN_LDC:
	{
		struct vi_chn_ldc_cfg cfg;

		rc = vi_get_chn_ldc_attr(p->sdk_cfg.pipe, p->sdk_cfg.chn, &cfg);

		if (copy_to_user(p->sdk_cfg.ptr, &cfg, sizeof(struct vi_chn_ldc_cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_ldc_cfg copy to user fail.\n");
			rc = -1;
			break;
		}

		break;
	}
	case VI_SDK_SET_CHN_FLIP_MIRROR:
	{
		struct vi_chn_flip_mirror_cfg cfg;

		if (copy_from_user(&cfg, p->sdk_cfg.ptr, sizeof(cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_flip_mirror_cfg copy from user fail.\n");
			break;
		}

		rc = vi_set_chn_flip_mirror(p->sdk_cfg.pipe, p->sdk_cfg.chn, &cfg);
		break;
	}
	case VI_SDK_GET_CHN_FLIP_MIRROR:
	{
		struct vi_chn_flip_mirror_cfg cfg;

		rc = vi_get_chn_flip_mirror(p->sdk_cfg.pipe, p->sdk_cfg.chn, &cfg);

		if (copy_to_user(p->sdk_cfg.ptr, &cfg, sizeof(struct vi_chn_flip_mirror_cfg)) != 0) {
			vi_pr(VI_ERR, "vi_chn_flip_mirror_cfg copy to user fail.\n");
			rc = -1;
			break;
		}
		break;
	}
	case VI_SDK_ATTACH_VB_POOL:
	{
		struct vi_vb_pool_cfg cfg;
		int vi_pipe;
		int vi_chn;
		vb_pool vbp;

		if (copy_from_user(&cfg, p->sdk_cfg.ptr, sizeof(cfg)) != 0) {
			vi_pr(VI_ERR, "vi_attach_vb_pool copy from user fail.\n");
			break;
		}

		vi_pipe = cfg.vi_pipe;
		vi_chn = cfg.vi_chn;
		vbp = (vb_pool)cfg.vb_pool;

		rc = vi_attach_vb_pool(vi_pipe, vi_chn, vbp);
		break;
	}
	case VI_SDK_DETACH_VB_POOL:
	{
		struct vi_vb_pool_cfg cfg;
		int vi_pipe;
		int vi_chn;

		if (copy_from_user(&cfg, p->sdk_cfg.ptr, sizeof(cfg)) != 0) {
			vi_pr(VI_ERR, "vi_detach_vb_pool copy from user fail.\n");
			break;
		}

		vi_pipe = cfg.vi_pipe;
		vi_chn = cfg.vi_chn;

		rc = vi_detach_vb_pool(vi_pipe, vi_chn);
		break;
	}
	case VI_SDK_GET_PIPE_DUMP_ATTR:
	{
		vi_dump_attr_s dump_attr;

		memset(&dump_attr, 0, sizeof(dump_attr));

		rc = vi_get_pipe_dump_attr(p->sdk_cfg.pipe, &dump_attr);

		if (copy_to_user(p->sdk_cfg.ptr, &dump_attr, sizeof(vi_dump_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_attr_s copy to user fail.\n");
			rc = -1;
			break;
		}

		break;
	}
	case VI_SDK_SET_PIPE_DUMP_ATTR:
	{
		vi_dump_attr_s dump_attr;

		if (copy_from_user(&dump_attr, p->sdk_cfg.ptr, sizeof(vi_dump_attr_s)) != 0) {
			vi_pr(VI_ERR, "vi_pipe_attr_s copy from user fail.\n");
			break;
		}

		rc = vi_set_pipe_dump_attr(p->sdk_cfg.pipe, &dump_attr);
		break;
	}
	default:
		break;
	}

	return rc;
}


#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/sched/types.h>


#include <linux/comm_video.h>
#include <linux/comm_gdc.h>
#include <linux/ldc_uapi.h>
#include <linux/comm_buffer.h>
#include <linux/comm_buffer.h>

#include "ldc_debug.h"
#include "ldc_core.h"
#include "ldc_sdk.h"
#include "ldc.h"
#include "mesh.h"
#include "ldc_proc.h"
#include "vbq.h"

#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

typedef struct COORD2D_INT_HW {
	unsigned char xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

void mesh_gen_get_1st_size(size_s in_size, unsigned int *mesh_1st_size)
{
	unsigned int ori_src_width, ori_src_height, src_width_s1, src_height_s1;
	unsigned int dst_height_s1, dst_width_s1, num_tilex_s1, num_tiley_s1;

	if (!mesh_1st_size)
		return;

	ori_src_width = in_size.width;
	ori_src_height = in_size.height;

	// In LDC Processing, width & height  aligned to TILESIZE **
	src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	// modify frame size
	dst_height_s1 = src_height_s1;
	dst_width_s1 = src_width_s1;
	num_tilex_s1 = dst_width_s1 / TILESIZE;
	num_tiley_s1 = dst_height_s1 / TILESIZE;

	*mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE *
			 MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 *
			 4; // 4 = 4 knots in a mesh
}

static int init_ldc_param(const struct vb_s *vb_in, struct vb_s *vb_out
	, struct gdc_task_attr *task, pixel_format_e pix_format, unsigned long long mesh_addr
	, unsigned char isLatask, void *cb_param, unsigned int cb_param_size, rotation_e rotation)
{
	int i;
	void *_cb_param;

	memset(task, 0, sizeof(*task));
	// prepare the in/out image info of the gdc task.
	task->img_in.video_frame.pixel_format = pix_format;
	task->img_in.video_frame.width = vb_in->buf.size.width
		- vb_in->buf.offset_left - vb_in->buf.offset_right;
	task->img_in.video_frame.height = vb_in->buf.size.height
	       	- vb_in->buf.offset_top - vb_in->buf.offset_bottom;
	task->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		task->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		task->img_in.video_frame.length[i] = vb_in->buf.length[i];
		task->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	task->img_out.video_frame.pixel_format = pix_format;
	task->img_out.video_frame.width = vb_out->buf.size.width;
	task->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		task->img_out.video_frame.phyaddr[i] =
			vb_out->buf.phy_addr[i];
		task->img_out.video_frame.length[i] = vb_out->buf.length[i];
		task->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	task->img_out.video_frame.pts = task->img_in.video_frame.pts;
	task->img_out.video_frame.private_data = vb_out;

	switch (rotation) {
	default:
	case ROTATION_0:
		task->img_out.video_frame.offset_top =
			vb_in->buf.offset_top;
		task->img_out.video_frame.offset_left =
			vb_in->buf.offset_left;
		task->img_out.video_frame.offset_bottom =
			vb_in->buf.offset_bottom;
		task->img_out.video_frame.offset_right =
			vb_in->buf.offset_right;
		break;
	case ROTATION_90:
		task->img_out.video_frame.offset_top =
			vb_in->buf.offset_left;
		task->img_out.video_frame.offset_left =
			vb_in->buf.offset_bottom;
		task->img_out.video_frame.offset_bottom =
			vb_in->buf.offset_right;
		task->img_out.video_frame.offset_right =
			vb_in->buf.offset_top;
		break;
	case ROTATION_180:
		task->img_out.video_frame.offset_top =
			vb_in->buf.offset_bottom;
		task->img_out.video_frame.offset_left =
			vb_in->buf.offset_right;
		task->img_out.video_frame.offset_bottom =
			vb_in->buf.offset_top;
		task->img_out.video_frame.offset_right =
			vb_in->buf.offset_left;
		break;
	case ROTATION_270:
		task->img_out.video_frame.offset_top =
			vb_in->buf.offset_right;
		task->img_out.video_frame.offset_left =
			vb_in->buf.offset_top;
		task->img_out.video_frame.offset_bottom =
			vb_in->buf.offset_left;
		task->img_out.video_frame.offset_right =
			vb_in->buf.offset_bottom;
		break;
	}

	vb_out->buf.offset_top = task->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = task->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = task->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = task->img_out.video_frame.offset_right;
	vb_out->buf.pts = task->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table,
	       MO_TBL_SIZE);

	_cb_param = vmalloc(cb_param_size);
	if (!_cb_param) {
		TRACE_LDC(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		return -1;
	}

	memcpy(_cb_param, cb_param, cb_param_size);
	task->private_data[0] = mesh_addr;
	task->private_data[1] = isLatask;
	task->private_data[2] = (uintptr_t)_cb_param;
	task->reserved = GDC_MAGIC;

	return 0;
}

static int mesh_gdc_do_rot(struct ldc_vdev *wdev, struct vb_s *vb_in
	, pixel_format_e pix_format, unsigned long long mesh_addr, bool sync_io, void *cb_param
	, unsigned int cb_param_size, mod_id_e mod_id, rotation_e rotation)
{
	struct gdc_handle_data data;
	struct ldc_job *job;
	struct gdc_task_attr *ptask = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	struct _gdc_cb_param *p_cb_param;

	if (rotation == ROTATION_0 || rotation == ROTATION_180) {
		size_out = vb_in->buf.size;
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
		size_out.height = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
	}

	ptask = vmalloc(sizeof(*ptask));
	if (!ptask) {
		TRACE_LDC(DBG_ERR, "vmalloc failed\n");
		ret = ERR_GDC_NOMEM;
		goto FAIL_ALLOC;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, PIXEL_FORMAT_NV21, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
	blk = vb_get_block_with_id(wdev->vb_pool, buf_size, ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_LDC(DBG_ERR, "get vb fail\n");
		ret = ERR_GDC_NOBUF;
		goto FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(ID_GDC), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(pix_format, size_out, &vb_out->buf, vb_out->phy_addr, DEFAULT_ALIGN);

	TRACE_LDC(DBG_DEBUG, "GDC usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
		, GDC_USAGE_ROTATION, rotation, vb_in->phy_addr, vb_out->phy_addr);

	ret = init_ldc_param(vb_in, vb_out, ptask, pix_format, mesh_addr
		, true, cb_param, cb_param_size, rotation);
	if (ret) {
		TRACE_LDC(DBG_ERR, "init_ldc_param fail\n");
		goto FAIL_EXIT;
	}

	ret = ldc_begin_job(wdev, &data);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_begin_job fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}
	ptask->handle = data.handle;
	ptask->rotation = rotation;
	ret = ldc_add_rotation_task(wdev, ptask);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_rotation_task fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}

	job = (struct ldc_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_cb_param = (struct _gdc_cb_param *)cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = ldc_end_job(wdev, data.handle);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_end_job fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}

	vfree(ptask);
	return ret;

FAIL_EXIT:
	vb_release_block((vb_blk)blk);
FAIL_GET_VB:
	vfree(ptask);
FAIL_ALLOC:
	vb_release_block((vb_blk)vb_in);

	return ret;

}

static int mesh_dwa_do_ldc_fisheye_gridinfo(struct ldc_vdev *wdev, enum ldc_usage usage
	, grid_info_attr_s *pst_gridinfo_attr, struct vb_s *vb_in, pixel_format_e en_pixformat, unsigned long long mesh_addr
	, bool sync_io, void *cb_param, unsigned int cb_param_size, mod_id_e en_mod_id)
{
	struct gdc_handle_data data;
	struct gdc_task_attr *pst_task = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	void *_p_cb_param;
	unsigned char i;
	struct ldc_job *job;
	struct _gdc_cb_param *p_cb_param;

	pst_task = vmalloc(sizeof(*pst_task));
	if (!pst_task) {
		TRACE_LDC(DBG_ERR, "vmalloc failed\n");
		ret = ERR_GDC_NOMEM;
		goto FAIL_ALLOC;
	}

	if (pst_gridinfo_attr->grid_out.width && pst_gridinfo_attr->grid_out.height) {
		size_out.width = ALIGN(pst_gridinfo_attr->grid_out.width, DWA_ALIGNMENT);
		size_out.height = pst_gridinfo_attr->grid_out.height;
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.width;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, en_pixformat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->vb_pool, buf_size, ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_LDC(DBG_ERR, "get vb fail\n");
		vb_release_block((vb_blk)vb_in);
		ret = ERR_GDC_NOBUF;
		goto FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(en_mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(ID_GDC), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(en_pixformat, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	TRACE_LDC(DBG_DEBUG, "LDC usage(%d) src phy-addr(0x%llx) size(%d-%d) dst phy-addr(0x%llx) size(%d-%d)\n"
		, usage, vb_in->phy_addr, vb_in->buf.size.width, vb_in->buf.size.height
		, vb_out->phy_addr, size_out.width, size_out.height);

	ret = ldc_begin_job(wdev, &data);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_begin_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	memset(pst_task, 0, sizeof(*pst_task));
	pst_task->handle = data.handle;
	pst_task->rotation = ROTATION_0;

	// prepare the in/out image info of the gdc task.
	pst_task->img_in.video_frame.pixel_format = en_pixformat;
	pst_task->img_in.video_frame.width = vb_in->buf.size.width;
	pst_task->img_in.video_frame.height = vb_in->buf.size.height;
	pst_task->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		pst_task->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		pst_task->img_in.video_frame.length[i] = vb_in->buf.length[i];
		pst_task->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	pst_task->img_out.video_frame.pixel_format = en_pixformat;
	pst_task->img_out.video_frame.width = vb_out->buf.size.width;
	pst_task->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		pst_task->img_out.video_frame.phyaddr[i] = vb_out->buf.phy_addr[i];
		pst_task->img_out.video_frame.length[i] = vb_out->buf.length[i];
		pst_task->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	pst_task->img_out.video_frame.pts = pst_task->img_in.video_frame.pts;
	pst_task->img_out.video_frame.private_data = vb_out;

	switch (pst_task->rotation) {
	default:
	case ROTATION_0:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_right;
		break;
	case ROTATION_90:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_top;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_left;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_bottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.offset_top = pst_task->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = pst_task->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = pst_task->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = pst_task->img_out.video_frame.offset_right;
	vb_out->buf.pts = pst_task->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_p_cb_param = vmalloc(cb_param_size);
	if (!_p_cb_param) {
		TRACE_LDC(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		ret = ERR_GDC_NOMEM;
		goto FAIL_EXIT;
	}
	memcpy(_p_cb_param, cb_param, cb_param_size);
	pst_task->private_data[0] = mesh_addr;
	pst_task->private_data[1] = (__u64)true;
	pst_task->private_data[2] = (uintptr_t)_p_cb_param;
	pst_task->reserved = GDC_MAGIC;

	if (usage == LDC_USAGE_LDC)
		ret = ldc_add_ldc_task(wdev, pst_task);
	else
		ret = ldc_add_cor_task(wdev, pst_task);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_xxx_tsk fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	job = (struct ldc_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = en_mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_cb_param = (struct _gdc_cb_param *)_p_cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = ldc_end_job(wdev, data.handle);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_end_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	vfree(pst_task);
	return ret;

FAIL_EXIT:
	vb_release_block((vb_blk)blk);
FAIL_GET_VB:
	vfree(pst_task);
FAIL_ALLOC:
	vb_release_block((vb_blk)vb_in);

	return ret;
}

static int mesh_dwa_do_ldc_fisheye(struct ldc_vdev *wdev, enum ldc_usage usage, struct vb_s *vb_in
	, pixel_format_e pix_format, unsigned long long mesh_addr, bool sync_io, void *cb_param
	, unsigned int cb_param_size, mod_id_e mod_id, rotation_e rotation)
{
	struct gdc_handle_data data;
	struct gdc_task_attr *ptask = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	rotation_e rotationOut;
	void *_cb_param;
	unsigned char i;
	struct ldc_job *job;
	struct _gdc_cb_param *p_cb_param;

	ptask = vmalloc(sizeof(struct gdc_task_attr));
	if (!ptask) {
		TRACE_LDC(DBG_ERR, "vmalloc failed\n");
		ret = ERR_GDC_NOMEM;
		goto FAIL_ALLOC;
	}

	if (rotation == ROTATION_0 || rotation == ROTATION_180) {
		size_out.width = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.height;
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.width;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->vb_pool, buf_size, ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_LDC(DBG_ERR, "get vb fail\n");
		vb_release_block((vb_blk)vb_in);
		ret = ERR_GDC_NOBUF;
		goto FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(ID_GDC), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(pix_format, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	TRACE_LDC(DBG_DEBUG, "DWA usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
	, usage, rotation, vb_in->phy_addr, vb_out->phy_addr);

	ret = ldc_begin_job(wdev, &data);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_begin_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	memset(ptask, 0, sizeof(*ptask));
	ptask->handle = data.handle;
	ptask->rotation = rotationOut;

	// prepare the in/out image info of the gdc task.
	ptask->img_in.video_frame.pixel_format = pix_format;
	ptask->img_in.video_frame.width = vb_in->buf.size.width;
	ptask->img_in.video_frame.height = vb_in->buf.size.height;
	ptask->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		ptask->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		ptask->img_in.video_frame.length[i] = vb_in->buf.length[i];
		ptask->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	ptask->img_out.video_frame.pixel_format = pix_format;
	ptask->img_out.video_frame.width = vb_out->buf.size.width;
	ptask->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		ptask->img_out.video_frame.phyaddr[i] = vb_out->buf.phy_addr[i];
		ptask->img_out.video_frame.length[i] = vb_out->buf.length[i];
		ptask->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	ptask->img_out.video_frame.pts = ptask->img_in.video_frame.pts;
	ptask->img_out.video_frame.private_data = vb_out;

	switch (rotation) {
	default:
	case ROTATION_0:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_right;
		break;
	case ROTATION_90:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_top;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_left;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_bottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.offset_top = ptask->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = ptask->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = ptask->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = ptask->img_out.video_frame.offset_right;
	vb_out->buf.pts = ptask->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_cb_param = vmalloc(cb_param_size);
	if (!_cb_param) {
		TRACE_LDC(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		ret = ERR_GDC_NOMEM;
		goto FAIL_EXIT;
	}

	memcpy(_cb_param, cb_param, cb_param_size);
	ptask->private_data[0] = mesh_addr;
	ptask->private_data[1] = (__u64)true;
	ptask->private_data[2] = (uintptr_t)_cb_param;
	ptask->reserved = GDC_MAGIC;

	if (usage == LDC_USAGE_LDC)
		ret = ldc_add_ldc_task(wdev, ptask);
	else
		ret = ldc_add_cor_task(wdev, ptask);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_xxx_tsk fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	job = (struct ldc_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_cb_param = (struct _gdc_cb_param *)_cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = ldc_end_job(wdev, data.handle);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_end_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto FAIL_EXIT;
	}

	vfree(ptask);
	return ret;

FAIL_EXIT:
	vb_release_block((vb_blk)blk);
FAIL_GET_VB:
	vfree(ptask);
FAIL_ALLOC:
	vb_release_block((vb_blk)vb_in);

	return ret;
}

int mesh_gdc_do_op(struct ldc_vdev *wdev, enum gdc_usage usage
	, const void *usage_param, struct vb_s *vb_in, pixel_format_e pix_format
	, unsigned long long mesh_addr, unsigned char sync_io, void *cb_param, unsigned int cb_param_size
	, mod_id_e mod_id, rotation_e rotation)
{
	int ret = ERR_GDC_ILLEGAL_PARAM;
	ldc_attr_s ldc_attr;
	fisheye_attr_s fisheye_attr;

	TRACE_LDC(DBG_DEBUG, "GDC usage(%d) rotation(%d), mesh-addr(0x%llx), cb_param_size(%d)\n",
			usage, rotation, (unsigned long long)mesh_addr, cb_param_size);

	if (usage != GDC_USAGE_ROTATION) {
		ret = ldc_check_null_ptr(usage_param);
		if (ret)
			return ret;
	}

	if (mesh_addr < DEFAULT_MESH_PADDR) {
		TRACE_LDC(DBG_ERR, "GDC mod(0x%x) usage(%d) mesh-addr(0x%llx) invalid\n",
			      mod_id, usage, (unsigned long long)mesh_addr);
		return -1;
	}

	switch (usage) {
	case GDC_USAGE_LDC:
		ldc_attr = *(ldc_attr_s *)usage_param;
		if (ldc_attr.grid_info_attr.enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, (enum ldc_usage)usage, &ldc_attr.grid_info_attr
				, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, (enum ldc_usage)usage, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	case GDC_USAGE_FISHEYE:
		fisheye_attr = *(fisheye_attr_s *)usage_param;
		if (fisheye_attr.grid_info_attr.enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, (enum ldc_usage)usage, &fisheye_attr.grid_info_attr
				, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, (enum ldc_usage)usage, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	case GDC_USAGE_ROTATION:
		ret = mesh_gdc_do_rot(wdev, vb_in, pix_format, mesh_addr
			, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	default:
		break;
	}

	return ret;
}

#ifndef _LDC_SDK_H_
#define _LDC_SDK_H_

#include <linux/list.h>
#include <linux/comm_errno.h>
#include <linux/irqreturn.h>
#include "vb.h"
#include "base_cb.h"
#include "ldc_core.h"

#define DEFAULT_MESH_PADDR	0x80000000

enum ldc_op_id { LDC_OP_MESH_JOB = 0, LDC_OP_MAX };

int ldc_exec_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg);
int ldc_rm_cb(void);
int ldc_reg_cb(struct ldc_vdev *wdev);

void ldc_wkup_frm_done_work(void *data);

/* Begin a ldc job,then add task into the job,ldc will finish all the task in the job.
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int ldc_begin_job(struct ldc_vdev *wdev,
		  struct gdc_handle_data *phandle);

/* End a job,all tasks in the job will be submmitted to ldc
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int ldc_end_job(struct ldc_vdev *wdev, unsigned long long handle);

/* Cancel a job ,then all tasks in the job will not be submmitted to ldc
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int ldc_cancel_job(struct ldc_vdev *wdev, unsigned long long handle);
int ldc_get_work_job(struct ldc_vdev *wdev, struct gdc_handle_data *data);

/* Add a rotation task to a ldc job
 *
 * @param phandle: unsigned long long *phandle
 * @param ptask: to describe what to do
 * @param rotation: for further settings
 * @return Error code (0 if successful)
 */
int ldc_add_rotation_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_ldc_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_cor_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_warp_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_affine_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_ldc_ldc_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_add_dwa_rot_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr);
int ldc_get_chn_frame(struct ldc_vdev *wdev, struct gdc_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec);

int ldc_attach_vb_pool(vb_pool vb_pool);
int ldc_detach_vb_pool(void);


/* set meshsize for rotation only
 *
 * @param nMeshHor: mesh counts horizontal
 * @param nMeshVer: mesh counts vertical
 * @return Error code (0 if successful)
 */
int ldc_set_mesh_size(int nMeshHor, int nMeshVer);

int ldc_sw_init(struct ldc_vdev *wdev);
void ldc_sw_deinit(struct ldc_vdev *wdev);

int ldc_set_identity(struct ldc_vdev *wdev,
				  struct gdc_identity_attr *attr);
int ldc_suspend_handler(void);
int ldc_resume_handler(void);

#define LDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV21) || (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_YUV_400))

#define DWA_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_444) ||           \
	 (fmt == PIXEL_FORMAT_RGB_888_PLANAR) || PIXEL_FORMAT_BGR_888_PLANAR || (fmt == PIXEL_FORMAT_YUV_400))

static inline int ldc_check_null_ptr(const void *ptr)
{
	if (!ptr) {
		TRACE_LDC(DBG_ERR, "NULL pointer\n");
		return ERR_GDC_NULL_PTR;
	}
	return 0;
}

static inline int check_ldc_format(video_frame_info_s imgIn, video_frame_info_s imgOut, unsigned long long mesh_addr)
{
	if (imgIn.video_frame.pixel_format !=
		imgOut.video_frame.pixel_format) {
		TRACE_LDC(DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",
			imgIn.video_frame.pixel_format,
			imgOut.video_frame.pixel_format);
		return ERR_GDC_ILLEGAL_PARAM;
	}
	if (!mesh_addr || mesh_addr == DEFAULT_MESH_PADDR) {
		if (!LDC_SUPPORT_FMT(imgIn.video_frame.pixel_format)) {
			TRACE_LDC(DBG_ERR, "pixelformat(%d) unsupported\n",
				imgIn.video_frame.pixel_format);
			return ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (!DWA_SUPPORT_FMT(imgIn.video_frame.pixel_format)) {
			TRACE_LDC(DBG_ERR, "pixelformat(%d) unsupported\n",
				imgIn.video_frame.pixel_format);
			return ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline int ldc_check_param_is_valid(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	int ret = 0;

	ret = ldc_check_null_ptr(wdev);
	if (ret)
		return false;

	ret = ldc_check_null_ptr(attr);
	if (ret)
		return false;

	ret = check_ldc_format(attr->img_in, attr->img_out, attr->private_data[0]);
	if (ret)
		return false;

	return true;
}

static inline int ldc_rot_check_size(rotation_e rotation, const struct gdc_task_attr *ptask)
{
	if (rotation > ROTATION_XY_FLIP) {
		TRACE_LDC(DBG_ERR, "invalid rotation(%d).\n", rotation);
		return ERR_GDC_ILLEGAL_PARAM;
	}

	if (rotation == ROTATION_90 || rotation == ROTATION_270 || rotation == ROTATION_XY_FLIP) {
		if (ptask->img_out.video_frame.width < ptask->img_in.video_frame.height) {
			TRACE_LDC(DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				rotation, ptask->img_out.video_frame.width,
				ptask->img_in.video_frame.height);
			return ERR_GDC_ILLEGAL_PARAM;
		}
		if (ptask->img_out.video_frame.height < ptask->img_in.video_frame.width) {
			TRACE_LDC(DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				rotation, ptask->img_out.video_frame.height,
				ptask->img_in.video_frame.width);
			return ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (ptask->img_out.video_frame.width < ptask->img_in.video_frame.width) {
			TRACE_LDC(DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				rotation, ptask->img_out.video_frame.width,
				ptask->img_in.video_frame.width);
			return ERR_GDC_ILLEGAL_PARAM;
		}
		if (ptask->img_out.video_frame.height < ptask->img_in.video_frame.height) {
			TRACE_LDC(DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				rotation, ptask->img_out.video_frame.height,
				ptask->img_in.video_frame.height);
			return ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return 0;
}


static inline unsigned char ldc_identity_is_match(gdc_identity_attr_s *attr_src, gdc_identity_attr_s *attr_dst)
{
	if ((attr_src->mod_id == attr_dst->mod_id)
		&& (attr_src->id == attr_dst->id)
		&& (strcmp(attr_src->name, attr_dst->name) == 0))
		return true;
	else
		return false;
}

#endif /* _LDC_SDK_H_ */

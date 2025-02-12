/* SPDX-License-Identifier: GPL-2.0 */
/*
 * plat_cam.h - Driver for KY X1 Platform Camera Manager
 *
 * Copyright(C) 2023 KY Micro Limited.
 */

#ifndef __PLAT_CAM_H__
#define __PLAT_CAM_H__

#include <media/v4l2-subdev.h>
#include <media/v4l2-ioctl.h>

struct spm_v4l2_subdev_ops {
	int (*registered)(struct v4l2_subdev *sd);
	void (*unregistered)(struct v4l2_subdev *sd);
	void (*notify)(struct v4l2_subdev *sd, unsigned int notification, void *arg);
};

/**
 * struct plat_cam_subdev - describes a camera subdevice
 *
 * @sd: 				   V4l2 subdevice
 * @ops:				   V4l2 subdecie operations
 * @internal_ops:		   V4l2 subdevice internal operations
 * @spm_ops:			   spm internal operations
 * @name:				   Name of the sub-device. Please notice that the name
 *							must be unique.
 * @sd_flags:			   Subdev flags. Can be:
 *							%V4L2_SUBDEV_FL_HAS_DEVNODE - Set this flag if
 *							this subdev needs a device node.
 *							%V4L2_SUBDEV_FL_HAS_EVENTS -  Set this flag if
 *							this subdev generates events.
 * @ent_function:		   Media entity function type.
 * @pads_cnt:			   Number of sink and source pads.
 * @pads:				   Pads array with the size defined by @pads_cnt.
 * @token:				   Pointer to cookie of the client driver
 *
 * Each instance of a subdev driver should create this struct, either
 * stand-alone or embedded in a larger struct. This structure should be
 * initialized/registered by plat_cam_register_subdev
 *
 */
struct plat_cam_subdev {
	struct v4l2_subdev				sd;
	const struct v4l2_subdev_ops			*ops;
	const struct v4l2_subdev_internal_ops		*internal_ops;
	const struct spm_v4l2_subdev_ops		*spm_ops;
	char						*name;
	uint32_t					sd_flags;
	uint32_t					ent_function;

	uint16_t					pads_cnt;
	struct media_pad				*pads;

	void						*token;
};

enum {
	PLAT_SD_NOTIFY_REGISTER_ISPFIRM = 1,
	PLAT_SD_NOTIFY_EIS_DATA,
	PLAT_SD_NOTIFY_SENSOR_STRM_CTRL,
	PLAT_SD_NOTIFY_REGISTER_SENSOR_OPS,
};

enum {
	SC_SENSOR_STRM_PAUSE = 0,
	SC_SENSOR_STRM_RESUME,
};

struct spm_camera_sensor_strm_ctrl {
	uint32_t	sensor_idx;
	uint32_t	cmd;
};

#define SC_ISPFIRM_CMD_CUSTOM				(1000)
#define SC_ISPFIRM_CMD_GET_FRAME_INFO			(SC_ISPFIRM_CMD_CUSTOM + 1)
#define SC_ISPFIRM_CMD_PIPE_RESET_START			(SC_ISPFIRM_CMD_CUSTOM + 2)
#define SC_ISPFIRM_CMD_PIPE_RESET_END			(SC_ISPFIRM_CMD_CUSTOM + 3)

#define SC_SENSOR_CMD_CUSTOM				(2000)
#define SC_SENSOR_CMD_STRM_CTRL				(SC_SENSOR_CMD_CUSTOM + 1)

struct spm_camera_sensor_ops {
	int (*send_cmd)(unsigned int cmd, void *cmd_payload, unsigned int payload_len);
};

struct spm_camera_ispfirm_ops {
	int (*send_cmd)(unsigned int cmd, void *cmd_payload, unsigned int payload_len);
	int (*irq_callback)(int irq_num, void *irq_data, unsigned int data_len);
};

struct camera_capture_slice_info {
	int32_t hw_pipe_id;
	int32_t total_slice_cnt;
	int32_t slice_width;
	int32_t raw_read_offset;
	int32_t yuv_out_offset;
	int32_t dwt_offset[4];
	int32_t exception_exit;	//stop this capture immediately because some error happen if true.
};

/**
 * struct spm_camera_vi_ops - vi to extern(isp) operation.
 *
 * @notify_caputre_until_done: the function pointer of notify vi to start capture and wait done whose
 *     return value indicates sucess(0) or fail(negative). Timeout is ms.
 */
struct spm_camera_vi_ops {
	int (*notify_caputre_until_done)(int slice_index, struct camera_capture_slice_info *slice_info, int timeout);
};

struct isp_firm {
	size_t frameinfo_size;
	struct spm_camera_ispfirm_ops *ispfirm_ops;
	struct spm_camera_vi_ops *vi_ops;
};

struct isp_eis_data {
	uint32_t pipeLineID;
	int32_t frameId;
	int32_t offsetX;
	int32_t offsetY;
};

enum {
	SC_FRM_INFO_T_INTERNAL = 0,
	SC_FRM_INFO_T_VRF,
};

struct frame_info {
	unsigned int pipe_id;
	unsigned int frame_id;
	int type;
	void *vaddr;
};

enum {
	ISP_IRQ,
	DMA_IRQ,
	CCIC_IRQ,
};

struct isp_pipe_reset {
	unsigned int pipe_id;
};

struct isp_irq_data {
	unsigned int pipe0_frame_id;
	unsigned int pipe1_frame_id;
	unsigned int pipe0_irq_status;
	unsigned int pipe1_irq_status;
};

struct dma_irq_data {
	unsigned int status1;
	unsigned int status2;
};

struct ccic_irq_data {
	unsigned int frame_id;
	unsigned int snapshot;
	unsigned int pipe_shadow_ready;
};

#define subdev_to_plat_csd(subdev) \
	container_of(subdev, struct plat_cam_subdev, sd)

unsigned long phys_cam2cpu(unsigned long phys_addr);
unsigned long phys_cpu2cam(unsigned long phys_addr);

int plat_cam_register_subdev(struct plat_cam_subdev *csd);

int plat_cam_unregister_subdev(struct plat_cam_subdev *csd);

struct v4l2_device *plat_cam_v4l2_device_get(void);

int plat_cam_v4l2_device_put(struct v4l2_device *v4l2_dev);

#endif /* ifndef __PLAT_CAM_H__ */

// SPDX-License-Identifier: GPL-2.0
/*
 * fe_isp.h - x1isp front end
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_ISP_IDI_H_
#define _KY_ISP_IDI_H_

#include <linux/types.h>
#include <media/media-entity.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <linux/platform_device.h>
#include "../subdev.h"
#include "../mlink.h"

#define FE_ISP_MAX_WIDTH	(5000)
#define FE_ISP_MAX_HEIGHT	(5000)

#define FE_ISP_MIN_WIDTH	(256)
#define FE_ISP_MIN_HEIGHT	(144)

enum {
	PIPELINE_ACTION_PIPE_ACK = PIPELINE_ACTION_CUSTOM_BASE + 1,
};

#define OFFLINE_CH_NUM	(2)
enum {
	OFFLINE_CH_PAD_IN = 0,
	OFFLINE_CH_PAD_P0OUT,
	OFFLINE_CH_PAD_P1OUT,
	OFFLINE_CH_PAD_NUM,
};

struct fe_x {
	struct spm_camera_subdev sc_subdev;
	struct notifier_block pipeline_notify_block;
};

struct fe_offline_channel {
	struct spm_camera_subdev sc_subdev;
	struct media_pad pads[OFFLINE_CH_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[OFFLINE_CH_PAD_NUM];
	int idx;
};

#define CSI_NUM	(3)
enum {
	CSI_PAD_IN = 0,
	CSI_PAD_RAWDUMP0,
	CSI_PAD_RAWDUMP1,
	CSI_PAD_PIPE0,
	CSI_PAD_PIPE1,
	CSI_PAD_AOUT,
	CSI_PAD_NUM
};
struct csi {
	struct spm_camera_subdev sc_subdev;
	struct notifier_block pipeline_notify_block;
	struct media_pad pads[CSI_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[CSI_PAD_NUM];
	int idx;
	int channel_type;
};

#define RAWDUMP_NUM	(2)
#define RAWDUMP_PAD_NUM	(2)
struct fe_rawdump {
	struct spm_camera_subdev sc_subdev;
	struct notifier_block pipeline_notify_block;
	struct media_pad pads[RAWDUMP_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[RAWDUMP_PAD_NUM];
	atomic_t close_done;
	int idx;
	int rawdump_only;
};

#define FORMATTER_NUM (2)
enum {
	FMT_PAD_IN = 0,
	FMT_PAD_AOUT,
	FMT_PAD_D1OUT,
	FMT_PAD_D2OUT,
	FMT_PAD_D3OUT,
	FMT_PAD_D4OUT,
	FORMATTER_PAD_NUM,
};

struct fe_formatter {
	struct spm_camera_subdev sc_subdev;
	struct media_pad pads[FORMATTER_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[FORMATTER_PAD_NUM];
	atomic_t dwt_refcnt;
	int idx;
};

#define DWT_NUM	(2)
#define DWT_LAYER_NUM	(4)
#define DWT_PAD_NUM	(2)
struct fe_dwt {
	struct spm_camera_subdev sc_subdev;
	struct media_pad pads[DWT_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[DWT_PAD_NUM];
	int idx;
	int layer_idx;
};

#define PIPE_NUM	(2)
enum {
	PIPE_PAD_IN = 0,
	PIPE_PAD_HDROUT,
	PIPE_PAD_F0OUT,
	PIPE_PAD_F1OUT,
	PIPE_PAD_F2OUT,
	PIPE_PAD_F3OUT,
	PIPE_PAD_F4OUT,
	PIPE_PAD_F5OUT,
	PIPE_PAD_RAWDUMP0OUT,
	PIPE_PAD_NUM,
};

struct fe_pipe {
	struct spm_camera_subdev sc_subdev;
	struct notifier_block pipeline_notify_block;
	struct media_pad pads[PIPE_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[PIPE_PAD_NUM];
	struct completion close_done;
	struct completion sde_sof;
	int idx;
};

#define HDR_COMBINE_NUM	(1)
enum {
	HDR_PAD_P0IN = 0,
	HDR_PAD_P1IN,
	HDR_PAD_F0OUT,
	HDR_PAD_F1OUT,
	HDR_PAD_F2OUT,
	HDR_PAD_F3OUT,
	HDR_PAD_F4OUT,
	HDR_PAD_F5OUT,
	HDR_COMBINE_PAD_NUM,
};

struct fe_hdr_combine {
	struct spm_camera_subdev sc_subdev;
	struct media_pad pads[HDR_COMBINE_PAD_NUM];
	struct v4l2_subdev_format pad_fmts[HDR_COMBINE_PAD_NUM];
};

static inline struct csi *v4l2_subdev_to_csi(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != MIPI || (SD_SUB(sd->grp_id) != CSI_MAIN && SD_SUB(sd->grp_id) != CSI_VCDT))
		return NULL;
	return (struct csi *)sd;
}

static inline struct csi *media_entity_to_csi(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_csi(sd);
}

static inline struct fe_hdr_combine *v4l2_subdev_to_hdr_combine(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || SD_SUB(sd->grp_id) != HDR_COMBINE)
		return NULL;
	return (struct fe_hdr_combine *)sd;
}

static inline struct fe_hdr_combine *media_entity_to_hdr_combine(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_hdr_combine(sd);
}

static inline struct fe_pipe *v4l2_subdev_to_pipe(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || SD_SUB(sd->grp_id) != PIPE)
		return NULL;
	return (struct fe_pipe *)sd;
}

static inline struct fe_pipe *media_entity_to_pipe(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_pipe(sd);
}

static inline struct fe_dwt *v4l2_subdev_to_dwt(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || (SD_SUB(sd->grp_id) != DWT0 && SD_SUB(sd->grp_id) != DWT1))
		return NULL;
	return (struct fe_dwt *)sd;
}

static inline struct fe_dwt *media_entity_to_dwt(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_dwt(sd);
}

static inline struct fe_formatter *v4l2_subdev_to_formatter(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || SD_SUB(sd->grp_id) != FORMATTER)
		return NULL;
	return (struct fe_formatter *)sd;
}

static inline struct fe_formatter *media_entity_to_formatter(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_formatter(sd);
}

static inline struct fe_rawdump *v4l2_subdev_to_rawdump(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || SD_SUB(sd->grp_id) != RAWDUMP)
		return NULL;
	return (struct fe_rawdump *)sd;
}

static inline struct fe_rawdump *media_entity_to_rawdump(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_rawdump(sd);
}

static inline struct fe_offline_channel *v4l2_subdev_to_offline_channel(struct v4l2_subdev *sd)
{
	if (SD_GRP(sd->grp_id) != FE_ISP || SD_SUB(sd->grp_id) != OFFLINE_CHANNEL)
		return NULL;
	return (struct fe_offline_channel *)sd;
}

static inline struct fe_offline_channel *media_entity_to_offline_channel(struct media_entity *me)
{
	struct v4l2_subdev *sd = NULL;
	if (!is_subdev(me))
		return NULL;

	sd = media_entity_to_v4l2_subdev(me);
	return v4l2_subdev_to_offline_channel(sd);
}

struct fe_pipe *fe_pipe_create(unsigned int grp_id, void *isp_ctx);
struct fe_rawdump *fe_rawdump_create(unsigned int grp_id, void *isp_ctx);
struct fe_offline_channel *fe_offline_channel_create(unsigned int grp_id,
						     void *isp_ctx);
struct fe_formatter *fe_formatter_create(unsigned int grp_id, void *isp_ctx);
struct fe_dwt *fe_dwt_create(unsigned int grp_id, void *isp_ctx);
struct fe_hdr_combine *fe_hdr_combine_create(unsigned int grp_id, void *isp_ctx);
struct csi *csi_create(unsigned int grp_id, void *isp_ctx);
// return isp context pointer
void *fe_isp_create_ctx(struct platform_device *pdev);
void fe_isp_release_ctx(void *isp_context);
int fe_isp_s_power(void *isp_context, int on);
#endif

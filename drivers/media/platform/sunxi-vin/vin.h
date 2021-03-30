
/*
 ******************************************************************************
 *
 * vin.h
 *
 * Hawkview ISP - vin.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/01	ISP Tuning Tools Support
 *
 *******************************************************************************
 */

#ifndef _VIN_H_
#define _VIN_H_

#include <media/media-device.h>
#include <media/media-entity.h>

#include "vin-video/vin_video.h"
#include "vin-video/vin_core.h"
#include "vin-csi/sunxi_csi.h"
#include "vin-isp/sunxi_isp.h"
#include "vin-vipp/sunxi_scaler.h"
#include "vin-mipi/sunxi_mipi.h"
#include "platform/platform_cfg.h"

#define VIN_MAX_DEV			4
#define VIN_MAX_CSI			2
#define VIN_MAX_CCI			2
#define VIN_MAX_MIPI			2
#define VIN_MAX_ISP			2
#define VIN_MAX_SCALER			4
#define VIN_MAX_CLK			1

struct vin_csi_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_mipi_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_cci_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_isp_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_stat_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_scaler_info {
	struct v4l2_subdev *sd;
	int id;
};

struct vin_clk_info {
	struct clk *clock;
	int use_count;
	unsigned long frequency;
};

struct vin_md {
	struct vin_csi_info csi[VIN_MAX_CSI];
	struct vin_mipi_info mipi[VIN_MAX_MIPI];
	struct vin_cci_info cci[VIN_MAX_CCI];
	struct vin_isp_info isp[VIN_MAX_ISP];
	struct vin_stat_info stat[VIN_MAX_ISP];
	struct vin_scaler_info scaler[VIN_MAX_SCALER];
	struct vin_core *vinc[VIN_MAX_DEV];
	struct vin_clk_info clk[VIN_MAX_CLK];
	bool isp_used;

	struct device *pmf;
	struct media_device media_dev;
	struct v4l2_device v4l2_dev;
	struct platform_device *pdev;
	struct vin_pinctrl {
		struct pinctrl *pinctrl;
	} pinctl;
	bool user_subdev_api;
	spinlock_t slock;
};

static inline struct vin_md *entity_to_vin_mdev(struct media_entity *me)
{
	return me->parent == NULL ? NULL :
		container_of(me->parent, struct vin_md, media_dev);
}

/*
 * Media pipeline operations to be called from within the video
 * node when it is the last entity of the pipeline. Implemented
 * by corresponding media device driver.
 */

struct vin_pipeline_ops {
	int (*open)(struct vin_pipeline *p, struct media_entity *me,
			  bool resume);
	int (*close)(struct vin_pipeline *p);
	int (*set_stream)(struct vin_pipeline *p, bool state);
};

#define vin_pipeline_call(f, op, p, args...)				\
	(!(f) ? -ENODEV : (((f)->pipeline_ops && (f)->pipeline_ops->op) ? \
			    (f)->pipeline_ops->op((p), ##args) : -ENOIOCTLCMD))


#endif /*_VIN_H_*/

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_MW_H_
#define _KY_MW_H_

#include <linux/of.h>
#include <linux/device.h>
#include <video/videomode.h>

#include <drm/drm_print.h>
#include <drm/drm_writeback.h>
#include <drm/drm_encoder.h>
#include <drm/drm_connector.h>
#include <drm/drm_bridge.h>

#include "ky_lib.h"

enum ky_wb_loc {
	KY_WB_COMP0 = 0,
	KY_WB_COMP1,
	KY_WB_COMP2,
	KY_WB_COMP3,
	KY_WB_COMP4,
	KY_WB_RCH0 = 8,
	KY_WB_RCH1,
	KY_WB_RCH2,
	KY_WB_RCH3,
	KY_WB_RCH4,
	KY_WB_RCH5,
	KY_WB_RCH6,
	KY_WB_RCH7,
	KY_WB_RCH8,
	KY_WB_RCH9,
	KY_WB_RCH10,
	KY_WB_RCH11,
	KY_WB_POST0 = 24,
	KY_WB_POST1,
	KY_WB_POST2,
};

struct ky_wb_device {
	uint32_t id;
	struct videomode vm;
	int status;
};

struct ky_wb {
	struct device dev;
	struct drm_encoder encoder;
	struct ky_wb_device ctx;
};

void saturn_wb_config(struct ky_dpu *dpu);

#endif

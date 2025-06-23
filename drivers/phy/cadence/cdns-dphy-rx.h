// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2022 Texas Instruments Incorporated - https://www.ti.com/
 */

#ifndef __CDNS_DPHY_RX_H__
#define __CDNS_DPHY_RX_H__

#include <media/v4l2-subdev.h>

#define DPHY_RX_LANES_MAX	4

struct cdns_dphy_rx {
	struct v4l2_subdev		subdev;
	void __iomem *regs;
	struct device *dev;
	struct phy *phy;

	struct clk				*psm_clk;
	struct clk				*apb_clk;
	struct reset_control 	*rst_dphy;
	struct reset_control 	*phy_cmnrst;

	u8				lanes[DPHY_RX_LANES_MAX];
	u8				num_lanes;
	u8				max_lanes;
	u8				id;

	struct v4l2_async_notifier	notifier;

	/* Remote source */
	struct v4l2_async_subdev asd;
	const void *drv_data;
	struct mutex mutex;
	atomic_t stream_cnt;
	struct platform_device *pdev;
	struct v4l2_mbus_framefmt format;
	union phy_configure_opts opts;
};

extern int cdns_dphy_rx_power_on(struct phy *phy);
extern int cdns_dphy_rx_power_off(struct phy *phy);
extern int cdns_dphy_rx_configure(struct phy *phy,
				  union phy_configure_opts *opts);
extern int cdns_dphy_rx_validate(struct phy *phy, enum phy_mode mode,
				 int submode, union phy_configure_opts *opts);


#endif
